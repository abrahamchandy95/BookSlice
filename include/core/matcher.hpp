#pragma once
#include <string>
#include <utility>
#include <vector>

class Matcher {
public:
  std::vector<std::pair<int, int>>
  matchIndices(const std::vector<std::string> &tocLines,
               const std::vector<std::string> &chapterLines,
               const std::string &chapterTitle) const;

private:
  bool skipLine(const std::string &line, const std::string &chapterTitle) const;

  int firstMatch(const std::vector<std::string> &chapterLines,
                 const std::string &line) const;

  static bool byChapterLine(const std::pair<int, int> &a,
                            const std::pair<int, int> &b) noexcept;
};
