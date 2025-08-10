#include "pdf/page_text.hpp"
#include <iostream>
#include <mupdf/fitz.h>

Handle<fz_page, PageDrop> makePage(fz_context *ctx, fz_document *doc,
                                   int index) noexcept {
  fz_page *raw = nullptr;
  if (ctx && doc) {
    fz_try(ctx) { raw = fz_load_page(ctx, doc, index); }
    fz_catch(ctx) {
      std::cerr << "fz_load_page failed: " << fz_caught_message(ctx) << '\n';
    }
  }
  return Handle<fz_page, PageDrop>(raw, PageDrop{ctx});
}

Handle<fz_buffer, BufferDrop> makeBuffer(fz_context *ctx,
                                         fz_page *page) noexcept {
  fz_buffer *raw = nullptr;
  if (ctx && page) {
    fz_try(ctx) { raw = fz_new_buffer_from_page(ctx, page, nullptr); }
    fz_catch(ctx) {
      std::cerr << "fz_new_buffer_from_page failed: " << fz_caught_message(ctx)
                << '\n';
    }
  }
  return Handle<fz_buffer, BufferDrop>(raw, BufferDrop{ctx});
}

std::string pageText(fz_context *ctx, fz_document *doc, int index) {
  auto page = makePage(ctx, doc, index);
  if (!page)
    return {};
  auto buf = makeBuffer(ctx, page.get());
  return std::string(bufferView(buf));
}
