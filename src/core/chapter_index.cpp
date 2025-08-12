#include "core/chapter_index.hpp"
#include "utils.hpp"
#include <cstddef>

std::vector<int>
ChapterIndex::indexChapters(const std::vector<std::string> &tocLines,
                            const std::vector<ChapterMatch> &files,
                            int startFrom) const {
  std::vector<int> positions(files.size(), -1);
  int cursor = startFrom;

  for (std::size_t k = 0; k < files.size(); ++k) {
    const std::string &key = files[k].key;

    int found = findFirstTocMatch(tocLines, key, cursor);

    if (found < 0) {
      const std::string noThe = stripLeadingThe(key);
      if (noThe.size() != key.size()) {
        found = findFirstTocMatch(tocLines, noThe, cursor);
      }
    }

    positions[k] = found;
    if (found >= 0)
      cursor = found + 1;
  }
  return positions;
}

int ChapterIndex::findFirstTocMatch(const std::vector<std::string> &tocLines,
                                    const std::string &key, int startAt) const {
  for (int i = startAt; i < static_cast<int>(tocLines.size()); ++i) {
    const std::string &ln = tocLines[i];
    if (!ln.empty() && ln.find(key) != std::string::npos)
      return i;
  }
  return -1;
}

std::string ChapterIndex::stripLeadingThe(const std::string &key) {
  std::string s1 = Title::stripLeadingThe(key);
  if (s1.size() != key.size())
    return s1;

  if (key.rfind("the", 0) == 0)
    return key.substr(3);

  return key;
}
