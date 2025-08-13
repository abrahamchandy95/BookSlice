#pragma once
#include <utility>
#include <vector>

#include "types.hpp"

class Segmenter {
public:
  std::vector<Section>
  buildSections(const std::vector<std::pair<int, int>> &matches, int totalLines,
                int minGap) const;

private:
  static std::vector<std::pair<int, int>>
  dedupeByLine(const std::vector<std::pair<int, int>> &matches);

  static std::vector<std::pair<int, int>>
  pickStarts(const std::vector<std::pair<int, int>> &ordered, int minGap);

  // helper to sort by line (second)
  static bool byLineAscending(const std::pair<int, int> &a,
                              const std::pair<int, int> &b) noexcept {
    return a.second < b.second;
  };
};
