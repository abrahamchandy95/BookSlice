
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <stdexcept>

#include "types.hpp"
#include "utils.hpp"

std::string Text::toLower(const std::string &s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return out;
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

std::string Text::collapseWhitespace(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  bool inSpace = false;
  for (unsigned char ch : s) {
    if (std::isspace(ch)) {
      if (!inSpace) {
        out.push_back(' ');
        inSpace = true;
      }
    } else {
      out.push_back(static_cast<char>(ch));
      inSpace = false;
    }
  }
  if (!out.empty() && out.front() == ' ')
    out.erase(out.begin());
  if (!out.empty() && out.back() == ' ')
    out.pop_back();
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

std::string Title::slugify(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s)
    out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
  return out;
}

bool Title::looksLikeTocName(const std::string &filename) {
  const std::string s = Text::normalizeStr(filename);
  static const std::array<const char *, 4> keys = {
      "table_of_contents", "tableofcontents", "contents", "toc"};
  for (auto k : keys) {
    if (s.find(Text::normalizeStr(k)) != std::string::npos)
      return true;
  }
  return false;
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
