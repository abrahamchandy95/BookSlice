#pragma once
#include <string>
#include <vector>

#include "types.hpp"

class ChapterIndex {
public:
  std::vector<int> indexChapters(const std::vector<std::string> &tocLines,
                                 const std::vector<ChapterMatch> &files,
                                 int startFrom = 0) const;

private:
  int findFirstTocMatch(const std::vector<std::string> &tocLines,
                        const std::string &key, int startAt) const;

  static std::string stripLeadingThe(const std::string &key);
};
