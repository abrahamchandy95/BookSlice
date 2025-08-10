#pragma once
#include <mupdf/fitz.h>
#include <vector>

#include "types.hpp"

std::vector<Outline> readOutline(fz_context *ctx, fz_document *doc,
                                 bool topLevelOnly = true);
