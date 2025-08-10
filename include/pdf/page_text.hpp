#pragma once
#include <mupdf/fitz.h>
#include <string_view>

#include "handle.hpp"

// Deleters carry the context to drop resources
struct PageDrop {
  fz_context *ctx{};
  void operator()(fz_page *p) const noexcept {
    if (p)
      fz_drop_page(ctx, p);
  }
};

struct BufferDrop {
  fz_context *ctx{};
  void operator()(fz_buffer *b) const noexcept {
    if (b)
      fz_drop_buffer(ctx, b);
  }
};

Handle<fz_page, PageDrop> makePage(fz_context *ctx, fz_document *doc,
                                   int index) noexcept;
Handle<fz_buffer, BufferDrop> makeBuffer(fz_context *ctx,
                                         fz_page *page) noexcept;

inline const char *bufferData(const Handle<fz_buffer, BufferDrop> &h) noexcept {
  return h ? reinterpret_cast<const char *>(h.get()->data) : nullptr;
}

inline std::size_t bufferSize(const Handle<fz_buffer, BufferDrop> &h) noexcept {
  return h ? static_cast<std::size_t>(h.get()->len) : 0u;
}

inline std::string_view
bufferView(const Handle<fz_buffer, BufferDrop> &h) noexcept {
  return {bufferData(h), bufferSize(h)};
}

std::string pageText(fz_context *ctx, fz_document *doc, int index);
