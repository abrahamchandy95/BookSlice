
#pragma once
#include <mupdf/fitz.h>
#include <string>
#include <vector>

#include "types.hpp"

std::vector<ChapterInfo> computeChapters(const std::vector<Outline> &outline,
                                         int totalPages);

std::string chapterText(fz_context *ctx, fz_document *doc,
                        const ChapterInfo &chapter);

void dumpChapters(fz_context *ctx, fz_document *doc,
                  const std::vector<ChapterInfo> &chapters,
                  const std::string &dir = "chapters");
