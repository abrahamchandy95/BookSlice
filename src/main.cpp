#include <filesystem>
#include <iostream>
#include <vector>

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

int main(int argc, char **argv) {
  const std::filesystem::path pdfPath =
      (argc > 1) ? std::filesystem::path{argv[1]}
                 : std::filesystem::path{"/home/workstation-tp/Downloads/"
                                         "Head-First-Design-Patterns.pdf"};

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

  int totalPages = pdf.pageCount();
  std::vector<ChapterInfo> chapters;
  if (!extractChapters(session, pdf, totalPages, chapters)) {
    std::cerr << "No TOC; skipping chapter extraction.\n";
    return 2;
  }

  const auto files = Catalog(kChaptersDir).collect();

  const std::filesystem::path tocPath = Title::findToc(kChaptersDir);
  if (tocPath.empty()) {
    std::cerr << "TOC text not found in " << kChaptersDir << "\n";
    return 1;
  }

  std::cout << "Extracting TOC windows from " << tocPath << '\n';
  sliceToc(tocPath, files, kMinLinesBetweenChapters, kTocDir);

  // Ensure output dir exists (non-throwing)
  std::error_code ec;
  std::filesystem::create_directories(kOutDir, ec);

  const auto tocLookup = TocLookup(kTocDir).build();
  if (tocLookup.empty()) {
    std::cerr << "No TOC slices found in " << kTocDir
              << " (did slicing produce any files?).\n";
    return 1;
  }

  SectionWriter writer({kMinLinesBetweenChapters, kOutDir});
  std::size_t written = 0;

  for (const auto &chapPath : FileIO::listChapters(kChaptersDir, ".txt")) {
    if (writer.runOne(chapPath, tocLookup))
      ++written;
  }

  std::cout << "\nDone — " << written
            << " chapter files processed; JSON saved in '" << kOutDir << "/'\n";
  return 0;
}
