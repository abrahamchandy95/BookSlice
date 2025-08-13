#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/matcher.hpp"
#include "core/segmenter.hpp"
#include "pdf/session.hpp"
#include "pipeline/catalog.hpp"
#include "pipeline/extract_chapters.hpp"
#include "pipeline/slice_toc.hpp"
#include "pipeline/toc_lookup.hpp"
#include "types.hpp"
#include "utils.hpp"

// ───────────────────────────  CONSTANTS  ──────────────────────────
constexpr std::string_view CHAPTERS_DIR = "chapters";
constexpr std::string_view TOC_DIR = "toc_sections";
constexpr std::string_view OUT_DIR = "chapter_segments";
constexpr int MIN_LINES_BETWEEN_CHAPTERS = 5;

struct Piece {
  std::string title;
  std::string body;
};

struct SectionJSON {
  std::string title;
  int start_line;
  std::string content;
};

bool processOneChapter(
    const std::filesystem::path &chapPath,
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
        &tocLookup) {
  std::string chapTitle = Title::extractChapterTitle(chapPath.string());
  auto it = tocLookup.find(chapTitle);
  if (it == tocLookup.end()) {
    std::cerr << "⚠️  no TOC found for chapter '" << chapTitle << "'\n";
    return false;
  }

  std::filesystem::path tocPath = it->second.front();
  auto tocLines = FileIO::readLines(tocPath);
  auto allLines = FileIO::readLines(chapPath);

  Matcher matcher;
  auto matches = matcher.matchIndices(tocLines, allLines, chapTitle);
  Segmenter segmenter;
  auto segments = segmenter.buildSections(
      matches, static_cast<int>(allLines.size()), MIN_LINES_BETWEEN_CHAPTERS);
  // Build JSON items
  nlohmann::json items = nlohmann::json::array();
  int sub_no = 1;
  for (auto [start, end, toc_idx] : segments) {
    std::string title = (toc_idx == -1)
                            ? "introduction"
                            : ("subsection" + std::to_string(sub_no++));
    std::string content;
    content.reserve(256);
    for (int i = start; i <= end; ++i) {
      content += allLines[i];
      if (i != end)
        content.push_back('\n');
    }
    content = Text::trim(content);

    items.push_back({{"title", title},
                     {"startline", start},
                     {"endline", end},
                     {"content", content}});
  }

  std::filesystem::create_directories(std::string(OUT_DIR));
  std::filesystem::path outPath = std::filesystem::path(std::string(OUT_DIR)) /
                                  (chapPath.stem().string() + "_segments.json");
  FileIO::writeJson(outPath, items);
  std::cout << "✓ " << items.size() << " segments → " << outPath << '\n';
  return true;
}

int main() {
  const std::string path =
      "/home/workstation-tp/Downloads/Head-First-Design-Patterns.pdf";

  PdfSession session;
  if (!session.isValid()) {
    std::cerr << "Invalid MuPDF session." << std::endl;
    return 1;
  }

  PdfFile pdf(session.ctx(), path);
  if (!pdf.isValid()) {
    std::cerr << "Invalid PDF file" << std::endl;
    return 1;
  }

  int totalPages = pdf.pageCount();
  std::vector<ChapterInfo> chapters;
  if (!extractChapters(session, pdf, totalPages, chapters)) {
    std::cerr << "No TOC; skipping chapter extraction.\n";
    return 2;
  }
  auto files = Catalog(std::string(CHAPTERS_DIR)).collect();

  auto tocPath = Title::findToc(std::string(CHAPTERS_DIR));
  if (tocPath.empty()) {
    std::cerr << "TOC text not found\n";
    return 1;
  }

  std::cout << "Extracting TOC windows from " << tocPath << '\n';
  sliceToc(tocPath, files, MIN_LINES_BETWEEN_CHAPTERS, std::string(TOC_DIR));
  std::filesystem::create_directories(OUT_DIR);

  auto tocLookup = TocLookup(std::string(TOC_DIR)).build();
  size_t written = 0;
  for (const auto &chapPath :
       FileIO::listChapters(std::string(CHAPTERS_DIR), ".txt")) {
    if (processOneChapter(chapPath, tocLookup))
      ++written;
  }

  std::cout << "\nDone — " << written
            << " chapter files processed; JSON saved in '" << OUT_DIR << "/'\n";
  return 0;
}
