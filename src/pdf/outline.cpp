#include <iostream>
#include <mupdf/fitz.h>

#include "pdf/outline.hpp"
#include "utils.hpp"

static void collectAll(fz_context *ctx, fz_outline *node,
                       std::vector<Outline> &out) {
  for (fz_outline *n = node; n; n = n->next) {
    if (n->title) {
      int page = n->page.page;
      if (page >= 0)
        out.push_back({Text::trim(n->title), page});
    }
    if (n->down)
      collectAll(ctx, n->down, out);
  }
}

static void collectTopOnly(fz_context *ctx, fz_outline *node,
                           std::vector<Outline> &out, int depth = 0) {
  for (fz_outline *n = node; n; n = n->next) {
    if (n->title) {
      int page = n->page.page;
      if (page >= 0 && depth == 0)
        out.push_back({Text::trim(n->title), page});
    }
    if (n->down)
      collectTopOnly(ctx, n->down, out, depth + 1);
  }
}

std::vector<Outline> readOutline(fz_context *ctx, fz_document *doc,
                                 bool topLevelOnly) {
  std::vector<Outline> entries;
  if (!ctx || !doc)
    return entries;

  fz_outline *root = nullptr;
  fz_try(ctx) { root = fz_load_outline(ctx, doc); }
  fz_catch(ctx) {
    std::cerr << "Failed to load outline: " << fz_caught_message(ctx) << '\n';
    return entries;
  }

  if (topLevelOnly)
    collectTopOnly(ctx, root, entries, 0);
  else
    collectAll(ctx, root, entries);

  if (root)
    fz_drop_outline(ctx, root);
  return entries;
}
