#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "core/chapter_indexer.hpp"
#include "types.hpp"

class SliceToc {
public:
  struct Config {
    int minLinesBetweenChapters{5};
    std::filesystem::path outDir{"toc_section"};
  };
  explicit SliceToc(Config cfg, ChapterIndex indexer = {})
      : cfg_(std::move(cfg)), indexer_(std::move(indexer)) {}

  std::size_t run(const std::filesystem::path &tocPath,
                  const std::vector<ChapterMatch> &files) const;

private:
  std::vector<std::string>
  readTocLines(const std::filesystem::path &tocPath) const;
  std::vector<std::string>
  normalize(const std::vector<std::string> &tocLines) const;
  std::vector<int>
  computePositions(const std::vector<std::string> &tocNorm,
                   const std::vector<ChapterMatch> &files) const;

  bool isValid(int start, int end) const noexcept;
  int writeSlice(const std::vector<std::string> &tocLines, int start, int end,
                 const std::filesystem::path &outFile) const;

private:
  Config cfg_;
  ChapterIndex indexer_;
};

// convenience wrapper
inline void sliceToc(const std::filesystem::path &tocPath,
                     const std::vector<ChapterMatch> &files,
                     int minLinesBetweenChapters,
                     const std::filesystem::path &outDir) {
  SliceToc slicer(SliceToc::Config{minLinesBetweenChapters, outDir});

  slicer.run(tocPath, files);
}
