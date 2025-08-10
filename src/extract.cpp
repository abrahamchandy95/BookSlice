#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mupdf/fitz.h>
#include <mupdf/fitz/context.h>
#include <mupdf/fitz/structured-text.h>
#include <ostream>

#include "extract.hpp"
#include "pdf/page_text.hpp"
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
      if (page >= 0 && depth == 0) {
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
  std::string out;
  const int count = ch.pageEnd - ch.pageStart + 1;
  if (count <= 0)
    return out;

  out.reserve(count * 1024);

  for (int p = ch.pageStart - 1; p <= ch.pageEnd - 1; ++p) {
    auto page = makePage(ctx, doc, p);
    if (!page) {
      std::cerr << "Skipping page " << (p + 1) << " (failed to load)\n";
      continue;
    }
    auto buf = makeBuffer(ctx, page.get());
    if (!buf) {
      std::cerr << "Skipping page " << (p + 1) << " (failed to render)\n";
      continue;
    }
    auto view = bufferView(buf);
    // C++20+: string::append(string_view) is fine; using data/size is explicit
    // & portable.
    out.append(view.data(), view.size());
    out.push_back('\n');
  }
  return out;
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
