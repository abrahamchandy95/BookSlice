#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mupdf/fitz.h>
#include <mupdf/fitz/structured-text.h>

#include "extract.hpp"
#include "utils.hpp"

static void collect(fz_context *ctx, fz_outline *node,
                    std::vector<Outline> &out) {
  for (fz_outline *n = node; n; n = n->next) {
    if (n->title) {
      int page = n->page.page;
      if (page >= 0)
        out.push_back({trim(n->title), page});
    }
    if (n->down)
      collect(ctx, n->down, out);
  }
}

static void collect_with_depth(fz_context *ctx, fz_outline *node,
                               std::vector<Outline> &out, int depth) {
  for (fz_outline *n = node; n; n = n->next) {
    if (n->title) {
      int page = n->page.page;
      if (page >= 0 && depth == 0) { // ONLY top-level entries
        out.push_back({trim(n->title), page});
      }
    }
    if (n->down)
      collect_with_depth(ctx, n->down, out, depth + 1);
  }
}

std::vector<Outline> OutlineParser::parse(fz_context *ctx, fz_document *doc) {
  std::vector<Outline> entries;
  fz_outline *outline{};
  fz_try(ctx) { outline = fz_load_outline(ctx, doc); }
  fz_catch(ctx) {
    std::cerr << "Failed to load outline: " << fz_caught_message(ctx) << '\n';
    return entries;
  }
  collect_with_depth(ctx, outline, entries, 0);
  if (outline)
    fz_drop_outline(ctx, outline);
  return entries;
}

// ─── ChapterUtils implementation ─────────────────────────────────────────────
namespace ChapterUtils {

std::vector<ChapterInfo> compute(const std::vector<Outline> &outline,
                                 int totalPages) {
  if (outline.empty())
    return {};

  auto sorted = outline;
  std::sort(sorted.begin(), sorted.end(),
            [](auto &a, auto &b) { return a.pageIndex < b.pageIndex; });

  std::vector<ChapterInfo> chapters;
  for (size_t i = 0; i < sorted.size(); ++i) {
    int start = sorted[i].pageIndex;
    int end =
        (i + 1 < sorted.size()) ? sorted[i + 1].pageIndex - 1 : totalPages - 1;
    if (end < start)
      end = start;

    chapters.push_back({sorted[i].title,
                        start + 1, // convert to 1-based
                        end + 1, end - start + 1});
  }
  return chapters;
}

} // namespace ChapterUtils

std::string extractChapterText(fz_context *ctx, fz_document *doc,
                               const ChapterInfo &ch) {
  std::string txt;
  for (int p = ch.pageStart - 1; p <= ch.pageEnd - 1; ++p) {
    fz_page *page = fz_load_page(ctx, doc, p);
    fz_buffer *buf = fz_new_buffer_from_page(ctx, page, nullptr);

    txt.append(reinterpret_cast<char *>(buf->data), buf->len);
    txt.push_back('\n');

    fz_drop_buffer(ctx, buf);
    fz_drop_page(ctx, page);
  }
  return txt;
}

void dumpChaptersToDir(fz_context *ctx, fz_document *doc,
                       const std::vector<ChapterInfo> &chapters,
                       const std::string &dir) {
  std::filesystem::create_directory(dir);

  for (size_t i = 0; i < chapters.size(); ++i) {
    const auto &ch = chapters[i];
    std::string body = extractChapterText(ctx, doc, ch);

    std::ostringstream name;
    name << dir << '/' << std::setw(2) << std::setfill('0') << (i + 1) << '_'
         << slugify(ch.title) << ".txt";

    std::ofstream(name.str()) << body;
    std::cout << "✓ saved " << name.str() << '\n';
  }
}
