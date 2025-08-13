#pragma once
#include "types.hpp"
#include <filesystem>
#include <vector>

// Catalogs chapter text files and produces normalized keys
class Catalog {
public:
  explicit Catalog(std::filesystem::path chaptersDir)
      : chaptersDir_(std::move(chaptersDir)) {}

  std::vector<ChapterMatch> collect() const;

private:
  std::filesystem::path chaptersDir_;
};
