#include "core/segmenter.hpp"
#include "types.hpp"
#include <algorithm>
#include <unordered_set>

std::vector<Section>
Segmenter::buildSections(const std::vector<std::pair<int, int>> &matches,
                         int totalLines, int minGap) const {
  if (matches.empty()) {
    return {Section{0, totalLines - 1, -1}};
  }

  std::vector<std::pair<int, int>> ordered = dedupeByLine(matches);

  std::vector<std::pair<int, int>> starts = pickStarts(ordered, minGap);

  std::vector<Section> segments;
  segments.reserve(starts.size() + 1);

  const int firstLine = starts.front().second;
  if (firstLine > 0) {
    segments.push_back(Section{0, firstLine - 1, -1});
  }

  for (size_t i = 0; i + 1 < starts.size(); ++i) {
    const auto &cur = starts[i];
    const auto &nxt = starts[i + 1];
    segments.push_back(Section{cur.second, nxt.second - 1, cur.first});
  }

  const auto &last = starts.back();
  segments.push_back(Section{last.second, totalLines - 1, last.first});

  return segments;
}

std::vector<std::pair<int, int>>
Segmenter::dedupeByLine(const std::vector<std::pair<int, int>> &matches) {
  std::unordered_set<int> seenLines;
  std::vector<std::pair<int, int>> out;
  out.reserve(matches.size());

  for (const auto &m : matches) {
    const int line = m.second;
    if (seenLines.insert(line).second) {
      out.push_back(m);
    }
  }
  std::sort(out.begin(), out.end(), byLineAscending);
  return out;
}

std::vector<std::pair<int, int>>
Segmenter::pickStarts(const std::vector<std::pair<int, int>> &ordered,
                      int minGap) {
  std::vector<std::pair<int, int>> starts;
  if (ordered.empty())
    return starts;

  starts.push_back(ordered.front());
  int lastLine = ordered.front().second;

  for (size_t i = 1; i < ordered.size(); ++i) {
    const auto &cur = ordered[i];
    if (cur.second - lastLine >= minGap) {
      starts.push_back(cur);
      lastLine = cur.second;
    }
  }
  return starts;
}
