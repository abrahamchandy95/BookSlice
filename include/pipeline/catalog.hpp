#pragma once
#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "types.hpp"
#include "utils.hpp"

// Catalogs chapter text files and produces normalized keys
class Catalog {
public:
  explicit Catalog(std::filesystem::path chaptersDir)
      : chaptersDir_(std::move(chaptersDir)) {}

  std::vector<ChapterMatch> collect() const {
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

private:
  std::filesystem::path chaptersDir_;
};

class TocLookup {
public:
  explicit TocLookup(std::filesystem::path tocSectionsDir)
      : tocSectionsDir_(std::move(tocSectionsDir)) {}

  std::unordered_map<std::string, std::vector<std::filesystem::path>>
  build() const {
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

private:
  std::filesystem::path tocSectionsDir_;
};
