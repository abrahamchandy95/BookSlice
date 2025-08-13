#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

// Builds a lookup: normalized chapter title -> TOC slice file paths
class TocLookup {
public:
  explicit TocLookup(std::filesystem::path tocSectionsDir)
      : tocSectionsDir_(std::move(tocSectionsDir)) {}

  std::unordered_map<std::string, std::vector<std::filesystem::path>>
  build() const;

private:
  std::filesystem::path tocSectionsDir_;
};
