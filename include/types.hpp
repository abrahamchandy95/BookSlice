#pragma once
#include <string>

struct Outline {
  std::string title;
  int pageIndex;
};

struct ChapterInfo {
  std::string title;
  int pageStart;
  int pageEnd;
  int pageCount;
};

struct ChapterMatch {
  std::string file;
  std::string key;
};
