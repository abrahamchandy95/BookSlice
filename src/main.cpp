#include <filesystem>
#include <iostream>
#include <vector>

#include "db/ingestor.hpp"
#include "db/mongo_conf.hpp"
#include "db/mongo_repo.hpp"
#include "pdf/metadata.hpp"
#include "pdf/session.hpp"
#include "pipeline/catalog.hpp"
#include "pipeline/extract_chapters.hpp"
#include "pipeline/section_writer.hpp"
#include "pipeline/slice_toc.hpp"
#include "types.hpp"
#include "utils.hpp"
// ───────────────────────────  CONSTANTS  ──────────────────────────
static const std::filesystem::path kChaptersDir{"chapters"};
static const std::filesystem::path kTocDir{"toc_sections"};
static const std::filesystem::path kOutDir{"chapter_segments"};
static constexpr int kMinLinesBetweenChapters = 5;

static bool extract_chapter_texts(const PdfSession &session, const PdfFile &pdf,
                                  int &totalPages,
                                  std::vector<ChapterInfo> &chapters) {
  totalPages = pdf.pageCount();
  if (!extractChapters(session, pdf, totalPages, chapters)) {
    std::cerr << "No TOC; skipping chapter extraction.\n";
    return false;
  }
  return true;
}

static bool slice_toc_windows() {
  const auto files = Catalog(kChaptersDir).collect();

  const std::filesystem::path tocPath = Title::findToc(kChaptersDir);
  if (tocPath.empty()) {
    std::cerr << "TOC text not found in " << kChaptersDir << "\n";
    return false;
  }

  std::cout << "Extracting TOC windows from " << tocPath << '\n';
  sliceToc(tocPath, files, kMinLinesBetweenChapters, kTocDir);
  return true;
}

static void ensure_out_dir() {
  std::error_code ec;
  std::filesystem::create_directories(kOutDir, ec);
}

static std::unordered_map<std::string, std::vector<std::filesystem::path>>
build_toc_lookup() {
  return TocLookup(kTocDir).build();
}

static std::size_t segment_all_chapters(
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
        &tocLookup) {
  SectionWriter writer({kMinLinesBetweenChapters, kOutDir});

  std::size_t written = 0;
  for (const auto &chapPath : FileIO::listChapters(kChaptersDir, ".txt")) {
    if (writer.runOne(chapPath, tocLookup))
      ++written;
  }
  return written;
}

static int run(const std::filesystem::path &pdfPath) {
  PdfSession session;
  if (!session.isValid()) {
    std::cerr << "Invalid MuPDF session.\n";
    return 1;
  }

  PdfFile pdf(session.ctx(), pdfPath.string());
  if (!pdf.isValid()) {
    std::cerr << "Invalid PDF file: " << pdfPath << "\n";
    return 1;
  }

  const BookTitle bt = getBookTitle(session.ctx(), pdf.doc(), pdfPath);
  std::cout << "Book Title ("
            << (bt.fromMetadata ? std::string("metadata: ") + bt.source
                                : "inferred")
            << "): " << bt.value << "\n";

  int totalPages = 0;
  std::vector<ChapterInfo> chapters;
  if (!extract_chapter_texts(session, pdf, totalPages, chapters)) {
    return 2;
  }

  if (!slice_toc_windows()) {
    return 3;
  }

  ensure_out_dir();

  const auto tocLookup = build_toc_lookup();
  if (tocLookup.empty()) {
    std::cerr << "No TOC slices found in " << kTocDir
              << " (did slicing produce any files?).\n";
    return 4;
  }

  const std::size_t written = segment_all_chapters(tocLookup);

  std::cout << "\nDone — " << written
            << " chapter files processed; JSON saved in '" << kOutDir << "/'\n";
  return 0;
}

static BookTitle fetch_book_title_for(const std::filesystem::path &pdfPath) {
  PdfSession session;
  if (!session.isValid()) {
    std::string base = pdfPath.stem().string();
    for (char &c : base)
      if (c == '_' || c == '-' || c == '.')
        c = ' ';
    base = Text::collapseWhitespace(base);
    base = Text::trim(base);
    return BookTitle{base, /*fromMetadata=*/false, "filename"};
  }
  PdfFile pdf(session.ctx(), pdfPath.string());
  if (!pdf.isValid()) {
    std::string base = pdfPath.stem().string();
    for (char &c : base)
      if (c == '_' || c == '-' || c == '.')
        c = ' ';
    base = Text::collapseWhitespace(base);
    base = Text::trim(base);
    return BookTitle{base, /*fromMetadata=*/false, "filename"};
  }
  return getBookTitle(session.ctx(), pdf.doc(), pdfPath);
}

int main(int argc, char **argv) {
  const char *home = std::getenv("HOME");
  if (!home) {
    std::cerr << "HOME not set!" << std::endl;
    return 1;
  }
  const std::filesystem::path pdfPath =
      (argc > 1) ? std::filesystem::path{argv[1]}
                 : std::filesystem::path(home) / "Downloads" /
                       "Head-First-Design-Patterns";
  const int pipeline_rc = run(pdfPath);
  if (pipeline_rc != 0)
    return pipeline_rc;
  const BookTitle bt = fetch_book_title_for(pdfPath);
  MongoConfig cfg;
  MongoRepository repo(cfg);
  Ingestor ingestor(repo);

  const int mongo_rc = ingestor.ingest_directory(kOutDir, pdfPath, bt);
  return mongo_rc;
}
