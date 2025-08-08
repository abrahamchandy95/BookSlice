
#include "utils.hpp"
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

std::string toLower(const std::string &s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return out;
}

bool isSpace(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

std::string trim(const std::string &s) {
  size_t i = 0, j = s.size();
  while (i < j && isSpace(s[i]))
    ++i;
  while (j > i && isSpace(s[j - 1]))
    --j;
  return s.substr(i, j - i);
}

std::string collapseWhitespace(const std::string &s) {
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

std::string slugify(const std::string &s) {
  std::string out;
  for (char c : s)
    out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
  return out;
}

void printOutline(const std::vector<Outline> &entries, int totalPages) {
  for (size_t i = 0; i < entries.size(); i++) {
    const std::string &chapter = entries[i].title;
    int startIndex = entries[i].pageIndex;
    int endIndex = (i + 1 < entries.size()) ? entries[i + 1].pageIndex - 1
                                            : totalPages - 1;

    if (endIndex < startIndex)
      endIndex = startIndex;

    int pageStart = startIndex + 1;
    int pageEnd = endIndex + 1;
    int pageCount = endIndex - startIndex + 1;

    std::cout << "'" << chapter << "': pages " << pageStart << "-" << pageEnd
              << " (" << pageCount << " pages)" << std::endl;
  }
}

std::string normalize_str(std::string s) {

  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  std::string out;
  for (unsigned char c : s)
    if (std::isalnum(c))
      out.push_back(c);
  return out;
}

std::vector<std::string> readLines(const std::filesystem::path &p) {
  std::vector<std::string> lines;
  std::ifstream f(p);
  if (!f) {
    throw std::runtime_error("readLines: failed to open " + p.string());
  }
  std::string s;
  while (std::getline(f, s)) {
    lines.emplace_back(std::move(s));
  }
  return lines;
}

void writeJson(const std::filesystem::path &outPath, const nlohmann::json &j) {
  const auto parent = outPath.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
  std::ofstream o(outPath);
  if (!o) {
    throw std::runtime_error("writeJson: failed to open " + outPath.string());
  }
  o << std::setw(2) << j; // pretty-print with 2-space indent
}
