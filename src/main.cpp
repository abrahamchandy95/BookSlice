
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "core.hpp"
#include "extract.hpp"
#include "utils.hpp"

struct Piece {
  std::string title;
  std::string body;
};

static std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

static bool iequals(const std::string &a, const std::string &b) {
  return to_lower(a) == to_lower(b);
}

static bool contains_ci(const std::string &hay, const std::string &needle) {
  return to_lower(hay).find(to_lower(needle)) != std::string::npos;
}

static bool looks_like_page_no(const std::string &s) {
  static const std::regex digits(R"(^\s*[0-9ivxlcdm]+\s*$)", std::regex::icase);
  return std::regex_match(s, digits);
}

static std::string jescape(const std::string &in) {
  std::string out;
  for (char c : in) {
    switch (c) {
    case '\\':
      out += "\\\\";
      break;
    case '\"':
      out += "\\\"";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      out.push_back(c);
    }
  }
  return out;
}

// TOC parsing
using SubList = std::vector<std::string>;
using TocMap = std::map<std::string, SubList>;

static TocMap parse_toc(const std::filesystem::path &tocFile,
                        const std::vector<ChapterInfo> &orderedChapters) {
  TocMap result;
  std::ifstream in(tocFile);
  if (!in) {
    std::cerr << "Cannot open " << tocFile << std::endl;
    return result;
  }
  std::vector<std::string> lines;
  for (std::string ln; std::getline(in, ln);)
    lines.push_back(trim(ln));

  std::size_t chapIdx = 0;
  std::size_t i = 0;
  while (i < lines.size() && chapIdx < orderedChapters.size()) {
    const auto &chapTitle = orderedChapters[chapIdx].title;
    while (i < lines.size() && !contains_ci(lines[i], chapTitle))
      ++i;
    std::size_t chapLine = i;
    std::size_t nextChapLine = lines.size();
    if (chapIdx + 1 < orderedChapters.size()) {
      const auto &nextTitle = orderedChapters[chapIdx + 1].title;
      std::size_t j = i + 1;
      while (j < lines.size() && !contains_ci(lines[j], nextTitle))
        j++;
      nextChapLine = j;
    }
    SubList subs;
    for (std::size_t k = chapLine + 1; k < nextChapLine; k++) {
      const std::string &ln = lines[k];
      if (ln.empty() || looks_like_page_no(ln))
        continue;
      subs.push_back(ln);
    }
    if (subs.size() >= 5)
      result[chapTitle] = std::move(subs);
    ++chapIdx;
    i = nextChapLine;
  }
  return result;
}

static std::vector<Piece> split_chapter(const std::filesystem::path &chapFile,
                                        const SubList &subsectionTitles) {
  std::vector<std::string> lines;
  {
    std::ifstream in(chapFile);
    for (std::string ln; std::getline(in, ln);)
      lines.push_back(ln);
  }
  std::size_t expect = 0;
  std::vector<Piece> pieces;
  std::ostringstream acc;
  std::string currentTitle =
      (subsectionTitles.empty() ? "Body" : subsectionTitles[0]);

  for (std::size_t idx = 0; idx < lines.size(); idx++) {
    std::string l = lines[idx];
    std::string l_lwr = to_lower(trim(l));
    if (expect < subsectionTitles.size() &&
        l_lwr.find(to_lower(subsectionTitles[expect])) != std::string::npos) {
      if (expect != 0) {
        pieces.push_back({subsectionTitles[expect - 1], acc.str()});
        acc.str("");
        acc.clear();
      }
      currentTitle = subsectionTitles[expect];
      ++expect;
    }
    acc << l << std::endl;
  }
  if (!acc.str().empty())
    pieces.push_back({currentTitle, acc.str()});
  return pieces;
}

static void write_chapter_json(const std::string &chapterTitle,
                               const std::vector<Piece> &pieces,
                               const std::string &dir = "structured") {
  std::filesystem::create_directory(dir);
  std::string file = dir + '/' + slugify(chapterTitle) + ".json";
  std::ofstream out(file);
  out << "{\n";
  out << "  \"chapter\": \"" << jescape(chapterTitle) << "\",\n";
  out << "  \"subsections\": [\n";
  for (std::size_t i = 0; i < pieces.size(); ++i) {
    out << "    {\n";
    out << "      \"title\": \"" << jescape(pieces[i].title) << "\",\n";
    out << "      \"body\": \"" << jescape(pieces[i].body) << "\"\n";
    out << "    }" << (i + 1 == pieces.size() ? "\n" : ",\n");
  }
  out << "  ]\n";
  out << "}\n";
  std::cout << "★ structured/" << slugify(chapterTitle) << ".json written\n";
}

int main() {
  const std::string path =
      "/home/workstation-tp/Downloads/Head-First-Design-Patterns.pdf";

  MuPdfEnvironment env;
  if (!env.isValid())
    return 1;

  PdfFile pdf(env.get(), path);
  if (!pdf.isValid())
    return 1;

  int totalPages = pdf.pageCount();
  auto outline = OutlineParser::parse(env.get(), pdf.get());

  if (outline.empty()) {
    std::cout << "(No outline found in this PDF.)" << std::endl;
    return 0;
  }
  printOutline(outline, totalPages);

  auto chapters = ChapterUtils::compute(outline, totalPages);
  dumpChaptersToDir(env.get(), pdf.get(), chapters);
  std::filesystem::path tocPath;
  for (const auto &f : std::filesystem::directory_iterator("chapters"))
    if (contains_ci(f.path().filename().string(), "table_of_contents"))
      tocPath = f.path();
  if (tocPath.empty()) {
    std::cerr << "No TOC file found in chapters/\n";
    return 0;
  }

  /* 4. Parse TOC to map chapter → subsection titles */
  TocMap tocData = parse_toc(tocPath, chapters);

  /* 5. For each chapter, split & emit JSON */
  for (const auto &ch : chapters) {
    std::filesystem::path chapFile;
    for (const auto &f : std::filesystem::directory_iterator("chapters"))
      if (contains_ci(f.path().filename().string(), ch.title)) {
        chapFile = f.path();
        break;
      }
    if (chapFile.empty()) {
      std::cerr << "Chapter file not found for \"" << ch.title << "\"\n";
      continue;
    }
    auto subs = tocData[ch.title]; // may be empty
    auto pieces = split_chapter(chapFile, subs);
    write_chapter_json(ch.title, pieces);
  }
  return 0;
}
