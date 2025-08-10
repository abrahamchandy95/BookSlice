
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "chapters.hpp"
#include "pdf/page_text.hpp"
#include "utils.hpp"

std::vector<ChapterInfo> computeChapters(const std::vector<Outline> &outline,
                                         int totalPages) {
  if (outline.empty())
    return {};

  auto sorted = outline;
  std::sort(sorted.begin(), sorted.end(),
            [](const Outline &a, const Outline &b) {
              return a.pageIndex < b.pageIndex;
            });

  std::vector<ChapterInfo> chapters;
  chapters.reserve(sorted.size());

  for (size_t i = 0; i < sorted.size(); ++i) {
    int start = sorted[i].pageIndex;
    int end =
        (i + 1 < sorted.size()) ? sorted[i + 1].pageIndex - 1 : totalPages - 1;
    if (end < start)
      end = start;

    chapters.push_back({sorted[i].title, start + 1, end + 1, end - start + 1});
  }
  return chapters;
}

std::string chapterText(fz_context *ctx, fz_document *doc,
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

    out.append(bufferView(buf));
    out.push_back('\n');
  }
  return out;
}

void dumpChapters(fz_context *ctx, fz_document *doc,
                  const std::vector<ChapterInfo> &chapters,
                  const std::string &dir) {
  std::filesystem::create_directories(dir);

  for (size_t i = 0; i < chapters.size(); ++i) {
    const auto &ch = chapters[i];
    std::string body = chapterText(ctx, doc, ch);

    std::ostringstream name;
    name << dir << '/' << std::setw(2) << std::setfill('0') << (i + 1) << '_'
         << slugify(ch.title) << ".txt";

    std::ofstream(name.str()) << body;
    std::cout << "✓ saved " << name.str() << '\n';
  }
}
