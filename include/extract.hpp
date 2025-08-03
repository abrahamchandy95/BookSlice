#pragma once
#include <mupdf/fitz/document.h>
#include <string>
#include <vector>

struct Outline {
  std::string title;
  int pageIndex; // 0-based
};

class OutlineParser {
public:
  static std::vector<Outline> parse(fz_context *ctx, fz_document *doc);
};

// ─── Chapter ranges ──────────────────────────────────────────────────────────
struct ChapterInfo {
  std::string title;
  int pageStart; // 1-based
  int pageEnd;   // 1-based
  int pageCount;
};

namespace ChapterUtils {
std::vector<ChapterInfo> compute(const std::vector<Outline> &outline,
                                 int totalPages);
}
std::string extractChapterText(fz_context *ctx, fz_document *doc,
                               const ChapterInfo &chapter);

void dumpChaptersToDir(fz_context *ctx, fz_document *doc,
                       const std::vector<ChapterInfo> &chapters,
                       const std::string &dir = "chapters");
