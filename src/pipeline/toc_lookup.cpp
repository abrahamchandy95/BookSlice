#include "pipeline/toc_lookup.hpp"
#include "utils.hpp"
#include <filesystem>

std::unordered_map<std::string, std::vector<std::filesystem::path>>
TocLookup::build() const {
  std::unordered_map<std::string, std::vector<std::filesystem::path>> lookup;

  for (const auto &entry :
       std::filesystem::directory_iterator(tocSectionsDir_)) {
    if (!entry.is_regular_file())
      continue;

    const auto &path = entry.path();
    const auto fname = path.filename().string();

    if (Title::isTocLabel(fname))
      continue;

    const auto title = Title::extractChapterTitle(path.string());
    if (Title::isTocLabel(title))
      continue;

    lookup[title].push_back(path);
  }

  return lookup;
}
