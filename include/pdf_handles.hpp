#pragma once
#include <mupdf/fitz.h>
#include <mupdf/fitz/buffer.h>
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

Handle<fz_page, PageDrop> make_page(fz_context *ctx, fz_document *doc,
                                    int index) noexcept;
Handle<fz_buffer, BufferDrop> make_buffer(fz_context *ctx,
                                          fz_page *page) noexcept;

inline const char *
buffer_data(const Handle<fz_buffer, BufferDrop> &h) noexcept {
  return h ? reinterpret_cast<const char *>(h.get()->data) : nullptr;
}

inline std::size_t
buffer_size(const Handle<fz_buffer, BufferDrop> &h) noexcept {
  return h ? static_cast<std::size_t>(h.get()->len) : 0u;
}

inline std::string_view
buffer_view(const Handle<fz_buffer, BufferDrop> &h) noexcept {
  return {buffer_data(h), buffer_size(h)};
}
