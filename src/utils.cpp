
#include <iostream>

#include "utils.hpp"

std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  size_t end = s.find_last_not_of(" \t\r\n");
  return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string slugify(const std::string &s) {
  std::string out;
  for (char c : s)
    out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
  return out;
}

void printOutline(const std::vector<Outline> &entries, int totalPages) {
  for (size_t i = 0; i < entries.size(); i++) {
    const std::string &chapter = entries[i].title;
    int startIndex = entries[i].pageIndex;
    int endIndex = (i + 1 < entries.size()) ? entries[i + 1].pageIndex - 1
                                            : totalPages - 1;

    if (endIndex < startIndex)
      endIndex = startIndex;

    int pageStart = startIndex + 1;
    int pageEnd = endIndex + 1;
    int pageCount = endIndex - startIndex + 1;

    std::cout << "'" << chapter << "': pages " << pageStart << "-" << pageEnd
              << " (" << pageCount << " pages)" << std::endl;
  }
}
