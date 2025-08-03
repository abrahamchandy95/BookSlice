
#include <iostream>
#include <string>

#include "core.hpp"
#include "extract.hpp"
#include "utils.hpp"

int main() {
  const std::string path =
      "/home/workstation-tp/Downloads/Head-First-Design-Patterns.pdf";

  MuPdfEnvironment env;
  if (!env.isValid())
    return 1;

  PdfFile pdf(env.get(), path);
  if (!pdf.isValid())
    return 1;

  int totalPages = pdf.pageCount();
  auto outline = OutlineParser::parse(env.get(), pdf.get());

  if (outline.empty()) {
    std::cout << "(No outline found in this PDF.)" << std::endl;
    return 0;
  }
  printOutline(outline, totalPages);

  auto chapters = ChapterUtils::compute(outline, totalPages);
  dumpChaptersToDir(env.get(), pdf.get(), chapters);

  return 0;
}
