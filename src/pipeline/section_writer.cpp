#include "pipeline/section_writer.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

#include "types.hpp"
#include "utils.hpp"

namespace {

struct SectionRow {
  std::string title;
  int startline{};
  int endline{};
  std::string content;
};

static void to_json(nlohmann::json &j, const SectionRow &s) {
  j = {{"title", s.title},
       {"startline", s.startline},
       {"endline", s.endline},
       {"content", s.content}};
}

static std::vector<SectionRow>
make_rows(const std::vector<Section> &segments,
          const std::vector<std::string> &lines) {
  std::vector<SectionRow> rows;
  rows.reserve(segments.size());
  int sub_no = 1;

  for (auto [start, end, toc_idx] : segments) {
    std::string title = (toc_idx == -1)
                            ? "introduction"
                            : ("subsection" + std::to_string(sub_no++));

    std::string content;
    content.reserve(256);
    for (int i = start; i <= end; ++i) {
      content += lines[i];
      if (i != end)
        content.push_back('\n');
    }

    rows.push_back(SectionRow{.title = std::move(title),
                              .startline = start,
                              .endline = end,
                              .content = Text::trim(content)});
  }
  return rows;
}

static nlohmann::json rows_to_json(const std::vector<SectionRow> &rows) {
  nlohmann::json::array_t arr;
  for (const auto &r : rows) {
    nlohmann::json jr;
    to_json(jr, r);
    arr.push_back(std::move(jr));
  }
  return nlohmann::json(std::move(arr));
}

} // namespace

bool SectionWriter::runOne(
    const std::filesystem::path &chapPath,
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
        &tocLookup) const {

  const std::string chapTitle = Title::extractChapterTitle(chapPath.string());
  auto it = tocLookup.find(chapTitle);
  if (it == tocLookup.end()) {
    std::cerr << "⚠️  no TOC found for chapter '" << chapTitle << "'\n";
    return false;
  }

  const auto tocPath = it->second.front();
  const auto tocLines = FileIO::readLines(tocPath);
  const auto allLines = FileIO::readLines(chapPath);

  const auto matches = matcher_.matchIndices(tocLines, allLines, chapTitle);
  const auto segments = segmenter_.buildSections(
      matches, static_cast<int>(allLines.size()), cfg_.minLinesBetweenChapters);

  const auto rows = make_rows(segments, allLines);
  const auto j = rows_to_json(rows);

  std::filesystem::create_directories(cfg_.outDir);
  const auto outPath =
      std::filesystem::path(cfg_.outDir) / (chapPath.stem().string() + ".json");
  FileIO::writeJson(outPath, j);

  std::cout << "✓ " << j.size() << " segments → " << outPath << '\n';
  return true;
}
