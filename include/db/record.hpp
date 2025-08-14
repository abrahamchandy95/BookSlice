#pragma once
#include <string>

struct Record {
  std::string book_title;
  std::string book_title_src;
  std::string book_path;

  std::string chapter_file;
  std::string chapter;
  std::string chapter_title;

  int section_index{};
  std::string title;
  int startline{};
  int endline{};
  std::string content;
};
