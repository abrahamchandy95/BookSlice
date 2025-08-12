#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/chapter_index.hpp"
#include "core/matcher.hpp"
#include "pdf/session.hpp"
#include "pipeline/extract_chapters.hpp"
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

std::unordered_map<std::string, std::vector<std::filesystem::path>>
buildTocLookup() {
  std::unordered_map<std::string, std::vector<std::filesystem::path>> lookup;

  for (const auto &entry : std::filesystem::directory_iterator(TOC_DIR)) {
    if (!entry.is_regular_file())
      continue;
    auto title = Title::extractChapterTitle(entry.path().string());
    if (title.find("table of contents") != std::string::npos)
      continue;
    lookup[title].push_back(entry.path());
  }
  return lookup;
}

std::vector<ChapterMatch> collect_chapter_matches() {
  std::vector<ChapterMatch> v;
  for (auto &f :
       std::filesystem::directory_iterator(std::string(CHAPTERS_DIR))) {
    if (!f.is_regular_file() || f.path().extension() != ".txt")
      continue;
    const auto fname = f.path().filename().string();
    if (Title::looksLikeTocName(fname))
      continue;
    auto t_norm = Title::extractChapterTitle(fname);

    auto key = Text::normalizeStr(t_norm);
    v.push_back({f.path().string(), key});
  }
  std::sort(v.begin(), v.end(),
            [](const ChapterMatch &a, const ChapterMatch &b) {
              return a.file < b.file;
            });
  return v;
}

std::vector<std::string> extract_between(const std::vector<std::string> &toc,
                                         const std::string &startKey,
                                         const std::string &endKey) {
  std::vector<std::string> buf;
  std::size_t i = 0, j = 0;
  while (j < toc.size()) {
    if (Text::normalizeStr(toc[j]).find(startKey) != std::string::npos) {
      i = j;
      ++j;
      while (j < toc.size()) {
        std::string cur = Text::normalizeStr(toc[j]);
        if (cur.find(endKey) != std::string::npos) {
          buf.insert(buf.end(), toc.begin() + i, toc.begin() + j);
          ++j;
          break;
        } else if (cur.find(startKey) != std::string::npos) {
          i = j;
        }
        ++j;
      }
    } else
      ++j;
  }
  return buf;
}

void extract_all_sections(const std::filesystem::path &tocPath,
                          const std::vector<ChapterMatch> &files,
                          const std::string &outDir = "toc_sections") {
  std::ifstream tocIn(tocPath);
  if (!tocIn) {
    std::cerr << "Cannot open " << tocPath << '\n';
    return;
  }

  std::vector<std::string> tocLines;
  for (std::string ln; std::getline(tocIn, ln);)
    tocLines.push_back(Text::trim(ln));

  auto tocNorm = Text::normalizeLines(tocLines);

  std::filesystem::create_directories(outDir);
  ChapterIndex indexer;
  auto positions = indexer.indexChapters(tocNorm, files);

  // Extract slices
  for (size_t idx = 0; idx + 1 < files.size(); ++idx) {
    int start = positions[idx];
    int end = positions[idx + 1];

    if (start < 0 || end < 0 || end <= start) {
      continue;
    }

    if ((end - start) < MIN_LINES_BETWEEN_CHAPTERS) {
      continue;
    }

    std::string stem = std::filesystem::path(files[idx].file).stem().string();
    std::filesystem::path out = std::filesystem::path(outDir) / (stem + ".txt");

    std::ofstream os(out);
    int written = 0;
    for (int i = start; i < end; ++i) {
      const auto &ln = tocLines[i];
      if (!ln.empty() && !Text::looksLikePageNo(ln)) {
        os << ln << '\n';
        ++written;
      }
    }

    std::cout << "◆ wrote " << written << " lines → " << out << "  (slice "
              << start << " .. " << (end - 1) << ")\n";
  }
}

using Segment = std::tuple<int, int, int>;

static std::vector<Segment>
segmentChapterWithToc(const std::vector<std::pair<int, int>> &matches,
                      int total_lines, int min_gap = 20) {
  // Python behavior: no matches ⇒ single intro slice
  if (matches.empty())
    return {std::make_tuple(0, total_lines - 1, -1)};

  // (0) sort-by-line is assumed by caller; drop duplicates on the same line
  std::unordered_set<int> seen_lines;
  std::vector<std::pair<int, int>> ordered;
  ordered.reserve(matches.size());
  for (auto [toc, ln] : matches) {
    if (seen_lines.insert(ln).second) {
      ordered.emplace_back(toc, ln);
    }
  }

  // (1) pick subsection starts using only the min-gap rule
  std::vector<std::pair<int, int>> starts; // (toc_idx, line_idx)
  starts.push_back(ordered.front());
  int last_line = ordered.front().second;

  for (size_t i = 1; i < ordered.size(); ++i) {
    auto [toc, ln] = ordered[i];
    if (ln - last_line >= min_gap) {
      starts.emplace_back(toc, ln);
      last_line = ln;
    }
  }

  // (2) turn starts into (start, end, toc) segments
  std::vector<Segment> segments;

  int first_ln = starts.front().second;
  if (first_ln > 0) {
    // introduction slice
    segments.emplace_back(0, first_ln - 1, -1);
  }

  for (size_t i = 0; i + 1 < starts.size(); ++i) {
    const auto &cur = starts[i];
    const auto &nxt = starts[i + 1];
    segments.emplace_back(cur.second, nxt.second - 1, cur.first);
  }

  const auto &last = starts.back();
  segments.emplace_back(last.second, total_lines - 1, last.first);

  return segments;
}

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
  auto segments =
      segmentChapterWithToc(matches, static_cast<int>(allLines.size()));

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
    return 2; // non-zero exit for shell scripts
  }
  auto files = collect_chapter_matches();
  if (files.size() < 2) {
    std::cerr << "need ≥2 chapter files\n";
    return 1;
  }

  auto tocPath = Title::findToc(std::string(CHAPTERS_DIR));
  if (tocPath.empty()) {
    std::cerr << "TOC text not found\n";
    return 1;
  }

  std::cout << "Extracting TOC windows from " << tocPath << '\n';
  extract_all_sections(tocPath, files);
  std::filesystem::create_directories(OUT_DIR);

  auto tocLookup = buildTocLookup();

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
