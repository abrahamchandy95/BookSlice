#pragma once
#include <filesystem>
#include <unordered_map>
#include <vector>

#include "core/matcher.hpp"
#include "core/segmenter.hpp"

// SectionWriter
// Orchestrates chapter segmentation and writes <chapter>_segments.json.
// Inputs: chapPath (chapter .txt), tocLookup (title -> toc-slice paths)
class SectionWriter {
public:
  struct Config {
    int minLinesBetweenChapters{5};
    std::filesystem::path outDir{"chapter_segments"};
  };

  explicit SectionWriter(Config cfg, Matcher matcher = {},
                         Segmenter segmenter = {})
      : cfg_(std::move(cfg)), matcher_(std::move(matcher)),
        segmenter_(std::move(segmenter)) {}

  // Returns true if JSON was written for this chapter.
  bool runOne(
      const std::filesystem::path &chapPath,
      const std::unordered_map<std::string, std::vector<std::filesystem::path>>
          &tocLookup) const;

private:
  Config cfg_;
  Matcher matcher_;
  Segmenter segmenter_;
};
