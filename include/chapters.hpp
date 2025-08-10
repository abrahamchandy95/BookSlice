
#pragma once
#include <mupdf/fitz.h>
#include <mupdf/fitz/context.h>
#include <mupdf/fitz/types.h>
#include <string>
#include <vector>

#include "types.hpp"

std::vector<ChapterInfo> computeChapters(const std::vector<Outline> &outline,
                                         int totalPages);

// get text from pdf
class ChapterReader {
public:
  ChapterReader(fz_context *ctx, fz_document *doc) noexcept
      : ctx_(ctx), doc_(doc) {
    assert(ctx_ && doc_);
  }
  std::string text(const ChapterInfo &chapter) const;
  fz_context *ctx() const noexcept { return ctx_; }
  fz_document *doc() const noexcept { return doc_; }

private:
  fz_context *ctx_ = nullptr;
  fz_document *doc_ = nullptr;
};

// where to put text
class ChapterWriter {
public:
  explicit ChapterWriter(std::string dir = "chapters") : dir_(std::move(dir)) {}

  std::size_t writeAll(const ChapterReader &reader,
                       const std::vector<ChapterInfo> &chapters) const;
  const std::string &dir() const noexcept { return dir_; }

private:
  std::string dir_;
};
