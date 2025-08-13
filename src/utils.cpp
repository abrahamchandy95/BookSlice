#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>

#include "types.hpp"
#include "utils.hpp"

// ───── Text ─────────────────────────────────────────────────────────────────
std::string Text::toLower(const std::string &s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return out;
}

bool Text::hasLetters(std::string_view s) noexcept {
  for (unsigned char c : s) {
    if (std::isalpha(c))
      return true;
  }
  return false;
}

double Text::upperRatio(std::string_view s) noexcept {
  int letters = 0, uppers = 0;
  for (unsigned char c : s) {
    if (std::isalpha(c)) {
      ++letters;
      if (std::isupper(c))
        ++uppers;
    }
  }
  return letters ? static_cast<double>(uppers) / letters : 0.0;
}

bool Text::isSpace(char c) noexcept {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

std::string Text::trim(const std::string &s) {
  size_t i = 0, j = s.size();
  while (i < j && isSpace(s[i]))
    ++i;
  while (j > i && isSpace(s[j - 1]))
    --j;
  return s.substr(i, j - i);
}

std::string Text::collapseWhitespace(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  bool inSpace = false;
  for (unsigned char ch : s) {
    if (std::isspace(ch)) {
      inSpace = true;
    } else {
      if (inSpace && !out.empty())
        out.push_back(' ');
      out.push_back(static_cast<char>(ch));
      inSpace = false;
    }
  }
  return out;
}

std::string Text::normalizeStr(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s)
    if (std::isalnum(c))
      out.push_back(c);
  return out;
}

bool Text::contains(std::string_view hay, std::string_view needle) {
  const std::string n = Text::collapseWhitespace(needle);
  if (n.empty())
    return false;
  const std::string h = Text::collapseWhitespace(hay);
  return h.find(n) != std::string::npos;
}

bool Text::looksLikePageNo(const std::string &s) {
  static const std::regex digits(R"(^\s*[0-9ivxlcdm]+\s*$)", std::regex::icase);
  return std::regex_match(s, digits);
}

std::vector<std::string>
Text::normalizeLines(const std::vector<std::string> &lines) {
  std::vector<std::string> out;
  out.reserve(lines.size());
  for (const auto &s : lines)
    out.push_back(normalizeStr(s));
  return out;
}

// ───── Title ─────────────────────────────────────────────────────────────────
void Title::replaceAll(std::string &s, const std::string &from,
                       const std::string &to) {
  if (from.empty())
    return;
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.size(), to);
    pos += to.size();
  }
}

std::string Title::stripLeadingChapterTag(const std::string &s) {
  static const std::regex re(
      R"(^(chapter|ch)\s+([0-9]+|[ivxlcdm]+)\s*[:.\-]?\s*)", std::regex::icase);
  return std::regex_replace(s, re, "");
}

std::string Title::stripLeadingThe(std::string s) {
  if (s.rfind("The ", 0) == 0)
    s.erase(0, 4);
  return s;
}

std::string Title::slugify(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s)
    out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
  return out;
}

std::filesystem::path Title::findToc(const std::filesystem::path &dir) {
  for (const auto &f : std::filesystem::directory_iterator(dir)) {
    auto name = f.path().filename().string();
    if (Title::isTocLabel(name))
      return f.path();
  }
  return {};
}

bool Title::containsAnyOf(std::string_view s,
                          std::span<const std::string_view> words) {
  for (auto w : words) {
    if (!w.empty() && s.find(w) != std::string::npos)
      return true;
  }
  return false;
}

bool Title::hasBannedWord(std::string_view s) {
  static const std::array<std::string_view, 4> kBanned{"download", "wowebook",
                                                       "copyright", "page"};
  return containsAnyOf(s, kBanned);
}

bool Title::isTocLabel(std::string_view text) {
  const std::string norm = Text::normalizeStr(std::string(text));
  static const std::array<std::string_view, 3> keys = {"tableofcontents",
                                                       "contents", "toc"};
  return Title::containsAnyOf(norm, keys);
}

bool Title::looksLikeTocName(const std::string &filename) {
  return Title::isTocLabel(filename);
}

std::string Title::extractChapterTitle(const std::string &path) {
  std::string name = std::filesystem::path(path).stem().string();
  name = std::regex_replace(name, std::regex(R"(^\d+_)"), "");
  Title::replaceAll(name, "__", "_");
  std::replace(name.begin(), name.end(), '_', ' ');
  name = Text::toLower(name);
  name = Text::collapseWhitespace(name);
  return Title::stripLeadingChapterTag(name);
}

bool Title::isSubtitleMatch(std::string_view tocLine,
                            std::string_view chapterLine) {
  std::string n = stripLeadingThe(std::string(tocLine));
  return Text::contains(chapterLine, n);
}

bool Title::hasChapterName(std::string_view s,
                           const std::string &chapterTitle) {
  std::string lower = Text::toLower(std::string(s));
  return Text::collapseWhitespace(lower).find(chapterTitle) !=
         std::string::npos;
}

bool Title::hasWeirdChars(std::string_view s) {
  return (std::string_view::npos != s.find("()")) ||
         (std::string_view::npos != s.find("//"));
}

bool Title::isNoisy(const std::string &s, const std::string &chapterTitle) {
  static constexpr double kUpperRatioCap = 0.6;
  if (hasChapterName(s, chapterTitle))
    return true;

  std::string lower = Text::toLower(s);
  if (hasBannedWord(lower))
    return true;
  if (hasWeirdChars(s))
    return true;

  if (!Text::hasLetters(s))
    return true;
  return Text::upperRatio(s) >= kUpperRatioCap;
}

// ───── FileIO ────────────────────────────────────────────────────────────────
std::vector<std::string> FileIO::readLines(const std::filesystem::path &p) {
  std::vector<std::string> lines;
  std::ifstream f(p);
  if (!f)
    throw std::runtime_error("readLines: failed to open " + p.string());
  std::string s;
  while (std::getline(f, s))
    lines.emplace_back(std::move(s));
  return lines;
}

std::vector<std::filesystem::path>
FileIO::listChapters(const std::filesystem::path &dir, std::string_view ext) {
  std::vector<std::filesystem::path> v;
  if (!std::filesystem::exists(dir))
    return v;
  for (const auto &e : std::filesystem::directory_iterator(dir)) {
    if (e.is_regular_file() && e.path().extension() == ext)
      v.push_back(e.path());
  }
  std::sort(v.begin(), v.end());
  return v;
}

void FileIO::writeJson(const std::filesystem::path &outPath,
                       const nlohmann::json &j) {
  const auto parent = outPath.parent_path();
  if (!parent.empty())
    std::filesystem::create_directories(parent);
  std::ofstream o(outPath);
  if (!o)
    throw std::runtime_error("writeJson: failed to open " + outPath.string());
  o << std::setw(2) << j;
}

// ───── OutlineView ───────────────────────────────────────────────────────────
void OutlineView::print(const std::vector<Outline> &entries, int totalPages) {
  for (size_t i = 0; i < entries.size(); i++) {
    const std::string &chapter = entries[i].title;
    int startIndex = entries[i].pageIndex;
    int endIndex = (i + 1 < entries.size()) ? entries[i + 1].pageIndex - 1
                                            : totalPages - 1;
    if (endIndex < startIndex)
      endIndex = startIndex;

    const int pageStart = startIndex + 1;
    const int pageEnd = endIndex + 1;
    const int pageCount = endIndex - startIndex + 1;

    std::cout << "'" << chapter << "': pages " << pageStart << "-" << pageEnd
              << " (" << pageCount << " pages)" << std::endl;
  }
}
