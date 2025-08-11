#include <iostream>

#include "chapters.hpp"
#include "pdf/outline.hpp"
#include "pipeline/extract_chapters.hpp"
#include "utils.hpp"

bool extractChapters(const PdfSession &session, const PdfFile &pdf,
                     int &totalPages, std::vector<ChapterInfo> &chapters,
                     bool topLevelOnly) {
  totalPages = pdf.pageCount();

  auto outline = readOutline(session.ctx(), pdf.doc(), topLevelOnly);
  if (outline.empty()) {
    std::cout << "No Table of Contents found in this pdf\n";
    chapters.clear();
    return false;
  }

  OutlineView::print(outline, totalPages);
  chapters = computeChapters(outline, totalPages);
  ChapterReader reader(session.ctx(), pdf.doc());
  ChapterWriter writer("chapters");
  const std::size_t written = writer.writeAll(reader, chapters);

  return written > 0;
}
