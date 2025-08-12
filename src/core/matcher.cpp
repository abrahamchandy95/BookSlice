#include "core/matcher.hpp"
#include "utils.hpp"
#include <algorithm>

std::vector<std::pair<int, int>>
Matcher::matchIndices(const std::vector<std::string> &tocLines,
                      const std::vector<std::string> &chapterLines,
                      const std::string &chapterTitle) const {
  std::vector<std::pair<int, int>> matches;

  for (int tocIndex = 0; tocIndex < static_cast<int>(tocLines.size());
       ++tocIndex) {
    const std::string &line = tocLines[tocIndex];
    if (skipLine(line, chapterTitle))
      continue;

    const int lineIndex = firstMatch(chapterLines, line);
    if (lineIndex >= 0) {
      matches.emplace_back(tocIndex, lineIndex);
    }
  }
  std::sort(matches.begin(), matches.end(), byChapterLine);
  return matches;
}

bool Matcher::skipLine(const std::string &line,
                       const std::string &chapterTitle) const {
  return Title::isNoisy(line, chapterTitle);
}

int Matcher::firstMatch(const std::vector<std::string> &chapterLines,
                        const std::string &line) const {
  for (int lineIndex = 0; lineIndex < static_cast<int>(chapterLines.size());
       ++lineIndex) {
    if (Title::isSubtitleMatch(line, chapterLines[lineIndex]))
      return lineIndex;
  }
  return -1;
}

bool Matcher::byChapterLine(const std::pair<int, int> &a,
                            const std::pair<int, int> &b) noexcept {
  return a.second < b.second;
}
