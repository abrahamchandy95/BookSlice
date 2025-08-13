#include "pipeline/catalog.hpp"
#include "utils.hpp"
#include <algorithm>
#include <filesystem>

// ----- ChapterCatalog --------------------------------------------------------
std::vector<ChapterMatch> Catalog::collect() const {
  std::vector<ChapterMatch> v;
  if (!std::filesystem::exists(chaptersDir_))
    return v;

  for (auto &f : std::filesystem::directory_iterator(chaptersDir_)) {
    if (!f.is_regular_file() || f.path().extension() != ".txt")
      continue;

    const auto fname = f.path().filename().string();
    if (Title::isTocLabel(fname))
      continue;

    const auto t_norm = Title::extractChapterTitle(fname);
    const auto key = Text::normalizeStr(t_norm);
    v.push_back({f.path().string(), key});
  }

  std::sort(v.begin(), v.end(),
            [](const ChapterMatch &a, const ChapterMatch &b) {
              return a.file < b.file;
            });
  return v;
}
