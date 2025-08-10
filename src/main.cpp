#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "extract.hpp"
#include "pdf/session.hpp"
#include "utils.hpp"

// ───────────────────────────  CONSTANTS  ──────────────────────────
constexpr std::string_view CHAPTERS_DIR = "chapters";
constexpr std::string_view TOC_DIR = "toc_sections";
constexpr std::string_view OUT_DIR = "chapter_segments";
constexpr int MIN_LINES_BETWEEN_CHAPTERS = 5;
constexpr double UPPER_RATIO_CAP = 0.6;

constexpr std::array<std::string_view, 4> BANNED_KEYWORDS{
    "download", "wowebook", "copyright", "page"};

struct Piece {
  std::string title;
  std::string body;
};

struct ChapterMatch {
  std::string file;
  std::string key;
};

struct SectionJSON {
  std::string title;
  int start_line;
  std::string content;
};

static bool looks_like_page_no(const std::string &s) {
  static const std::regex digits(R"(^\s*[0-9ivxlcdm]+\s*$)", std::regex::icase);
  return std::regex_match(s, digits);
}

static void replaceAll(std::string &s, const std::string &from,
                       const std::string &to) {
  if (from.empty())
    return;
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.size(), to);
    pos += to.size();
  }
}

static std::string strip_leading_chapter_tag(const std::string &s) {
  static const std::regex re(
      R"(^(chapter|ch)\s+([0-9]+|[ivxlcdm]+)\s*[:.\-]?\s*)", std::regex::icase);
  return std::regex_replace(s, re, "");
}

static bool looks_like_toc_name(const std::string &filename) {
  std::string s = normalize_str(filename);
  static const std::array<const char *, 4> keys = {
      "table_of_contents", "tableofcontents", "contents", "toc"};
  for (auto k : keys) {
    if (s.find(normalize_str(k)) != std::string::npos)
      return true;
  }
  return false;
}

static std::vector<std::string>
normalize_lines(const std::vector<std::string> &lines) {
  std::vector<std::string> out;
  out.reserve(lines.size());
  for (auto &s : lines) {
    out.push_back(normalize_str(s));
  }
  return out;
}

static std::vector<int>
locate_keys_in_toc_monotonic(const std::vector<std::string> &tocNorm,
                             const std::vector<ChapterMatch> &files,
                             int start_from = 0) {
  std::vector<int> pos(files.size(), -1);
  int cursor = start_from;

  for (size_t k = 0; k < files.size(); ++k) {
    const std::string &key = files[k].key;
    int found = -1;
    for (int i = cursor; i < (int)tocNorm.size(); ++i) {
      const auto &ln = tocNorm[i];
      if (!ln.empty() && ln.find(key) != std::string::npos) {
        found = i;
        break;
      }
    }
    // try matching without 'the'
    if (found < 0 && key.rfind("the", 0) == 0) {
      std::string noThe = key.substr(3);
      for (int i = cursor; i < (int)tocNorm.size(); ++i) {
        const auto &ln = tocNorm[i];
        if (!ln.empty() && ln.find(noThe) != std::string::npos) {
          found = i;
          break;
        }
      }
    }
    pos[k] = found;
    if (found >= 0) {
      cursor = found + 1;
    }
  }
  return pos;
}

static std::string extractChapterTitle(const std::string &path) {

  std::string name = std::filesystem::path(path).stem().string();
  name = std::regex_replace(name, std::regex(R"(^\d+_)"), "");
  replaceAll(name, "__", "_");
  std::replace(name.begin(), name.end(), '_', ' ');
  name = toLower(name);
  name = collapseWhitespace(name);
  return strip_leading_chapter_tag(name);
}

static bool isNoisy(const std::string &h,
                    const std::string &chapter_title_norm) {
  std::string low = toLower(h);
  if (collapseWhitespace(low).find(chapter_title_norm) != std::string::npos)
    return true;

  for (const auto &k : BANNED_KEYWORDS)
    if (low.find(k) != std::string::npos)
      return true;

  if (h.find("()") != std::string::npos || h.find("//") != std::string::npos)
    return true;

  int letters = 0, uppers = 0;
  for (unsigned char c : h) {
    if (std::isalpha(c)) {
      ++letters;
      if (std::isupper(c))
        ++uppers;
    }
  }
  if (letters == 0)
    return true;
  return static_cast<double>(uppers) / letters >= UPPER_RATIO_CAP;
}

std::unordered_map<std::string, std::vector<std::filesystem::path>>
buildTocLookup() {
  namespace fs = std::filesystem;
  std::unordered_map<std::string, std::vector<fs::path>> lookup;

  for (const auto &entry : fs::directory_iterator(TOC_DIR)) {
    if (!entry.is_regular_file())
      continue;
    auto title = extractChapterTitle(entry.path().string());
    if (title.find("table of contents") != std::string::npos)
      continue;
    lookup[title].push_back(entry.path());
  }
  return lookup;
}

static bool isSubstringWithArticleStripped(std::string s1,
                                           const std::string &s2) {
  if (s1.size() < 5)
    return false;

  if (s1.rfind("The ", 0) == 0)
    s1.erase(0, 4);

  s1 = collapseWhitespace(s1);
  std::string b = collapseWhitespace(s2);

  return b.find(s1) != std::string::npos;
}

static std::vector<std::pair<int, int>>
findMatchingIndices(const std::vector<std::string> &tocLines,
                    const std::vector<std::string> &allLines,
                    const std::string &chapter_title_norm) {
  std::vector<std::pair<int, int>> matches;
  for (int tIdx = 0; tIdx < static_cast<int>(tocLines.size()); ++tIdx) {
    if (isNoisy(tocLines[tIdx], chapter_title_norm))
      continue;

    for (int aIdx = 0; aIdx < static_cast<int>(allLines.size()); ++aIdx) {
      if (isSubstringWithArticleStripped(tocLines[tIdx], allLines[aIdx])) {
        matches.emplace_back(tIdx, aIdx);
        break;
      }
    }
  }

  std::sort(matches.begin(), matches.end(),
            [](auto a, auto b) { return a.second < b.second; });
  return matches;
}

void print_outline_and_dump(PdfSession &session, PdfFile &pdf, int &totalPages,
                            std::vector<ChapterInfo> &chapters) {
  totalPages = pdf.pageCount();
  auto outline = OutlineParser::parse(session.ctx(), pdf.doc());

  if (outline.empty()) {
    std::cout << "No Table of Contents found in this pdf" << std::endl;
    return;
  }
  printOutline(outline, totalPages);
  chapters = ChapterUtils::compute(outline,
                                   totalPages); // or your ChapterUtils::compute
  dumpChaptersToDir(session.ctx(), pdf.doc(), chapters);
}

std::vector<ChapterMatch> collect_chapter_matches() {
  std::vector<ChapterMatch> v;
  for (auto &f :
       std::filesystem::directory_iterator(std::string(CHAPTERS_DIR))) {
    if (!f.is_regular_file() || f.path().extension() != ".txt")
      continue;
    const auto fname = f.path().filename().string();
    if (looks_like_toc_name(fname))
      continue;
    auto t_norm = extractChapterTitle(fname);

    auto key = normalize_str(t_norm);
    v.push_back({f.path().string(), key});
  }
  std::sort(v.begin(), v.end(),
            [](const ChapterMatch &a, const ChapterMatch &b) {
              return a.file < b.file;
            });
  return v;
}

std::filesystem::path find_toc_path() {
  for (auto &f : std::filesystem::directory_iterator("chapters")) {
    auto name = f.path().filename().string();
    if (looks_like_toc_name(name)) {
      return f.path();
    }
  }
  return {};
}

std::vector<std::string> extract_between(const std::vector<std::string> &toc,
                                         const std::string &startKey,
                                         const std::string &endKey) {
  std::vector<std::string> buf;
  std::size_t i = 0, j = 0;
  while (j < toc.size()) {
    if (normalize_str(toc[j]).find(startKey) != std::string::npos) {
      i = j;
      ++j;
      while (j < toc.size()) {
        std::string cur = normalize_str(toc[j]);
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
    tocLines.push_back(trim(ln));

  auto tocNorm = normalize_lines(tocLines);

  std::filesystem::create_directories(outDir);

  auto positions = locate_keys_in_toc_monotonic(tocNorm, files);

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
      if (!ln.empty() && !looks_like_page_no(ln)) {
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
  std::string chapTitle = extractChapterTitle(chapPath.string());
  auto it = tocLookup.find(chapTitle);
  if (it == tocLookup.end()) {
    std::cerr << "⚠️  no TOC found for chapter '" << chapTitle << "'\n";
    return false;
  }

  std::filesystem::path tocPath = it->second.front();
  auto tocLines = readLines(tocPath);
  auto allLines = readLines(chapPath);

  auto matches = findMatchingIndices(tocLines, allLines, chapTitle);
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
    content = trim(content);

    items.push_back({{"title", title},
                     {"startline", start},
                     {"endline", end},
                     {"content", content}});
  }

  std::filesystem::create_directories(std::string(OUT_DIR));
  std::filesystem::path outPath = std::filesystem::path(std::string(OUT_DIR)) /
                                  (chapPath.stem().string() + "_segments.json");
  writeJson(outPath, items);
  std::cout << "✓ " << items.size() << " segments → " << outPath << '\n';
  return true;
}

static std::vector<std::filesystem::path> listChapterFilesSorted() {
  std::vector<std::filesystem::path> v;
  if (!std::filesystem::exists(std::string(CHAPTERS_DIR)))
    return v;
  for (const auto &e :
       std::filesystem::directory_iterator(std::string(CHAPTERS_DIR))) {
    if (e.is_regular_file() && e.path().extension() == ".txt")
      v.push_back(e.path());
  }
  std::sort(v.begin(), v.end());
  return v;
}

int main() {
  const std::string path =
      "/home/workstation-tp/Downloads/Head-First-Design-Patterns.pdf";

  PdfSession session;
  if (!session.isValid()) {
    std::cerr << "Invalid MuPDF session." << std::endl;
    return 1;
  }
  /*
    MuPdfEnvironment env;
  if (!env.isValid()) {
    std::cerr << "Invalid MuPDF environment." << std::endl;
    return 1;
  }

  PdfFile pdf(env.get(), path);
  if (!pdf.isValid()) {
    std::cerr << "Invalid PDF file" << std::endl;
    return 1;
  }
*/
  PdfFile pdf(session.ctx(), path);
  if (!pdf.isValid()) {
    std::cerr << "Invalid PDF file" << std::endl;
    return 1;
  }

  int totalPages = pdf.pageCount();
  std::vector<ChapterInfo> chapters;
  print_outline_and_dump(session, pdf, totalPages, chapters);

  auto files = collect_chapter_matches();
  if (files.size() < 2) {
    std::cerr << "need ≥2 chapter files\n";
    return 1;
  }

  auto tocPath = find_toc_path();
  if (tocPath.empty()) {
    std::cerr << "TOC text not found\n";
    return 1;
  }

  std::cout << "Extracting TOC windows from " << tocPath << '\n';
  extract_all_sections(tocPath, files);
  std::filesystem::create_directories(OUT_DIR);

  auto tocLookup = buildTocLookup();

  size_t written = 0;
  for (const auto &chapPath : listChapterFilesSorted()) {
    if (processOneChapter(chapPath, tocLookup))
      ++written;
  }

  std::cout << "\nDone — " << written
            << " chapter files processed; JSON saved in '" << OUT_DIR << "/'\n";
  return 0;
}
