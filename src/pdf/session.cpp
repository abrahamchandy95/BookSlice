#include <iostream>
#include <mupdf/fitz.h>
#include <string>

#include "pdf/session.hpp"

void FzContextDrop::operator()(fz_context *ctx) const noexcept {
  if (ctx)
    fz_drop_context(ctx);
}

void FzDocumentDrop::operator()(fz_document *doc) const noexcept {
  if (ctx && doc)
    fz_drop_document(ctx, doc);
}

// ----- PdfSession
PdfSession::PdfSession() noexcept {
  fz_context *raw = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
  if (!raw) {
    std::cerr << "Failed to create MuPDF context.\n";
    return;
  }
  ctx_ = Handle<fz_context, FzContextDrop>(raw, FzContextDrop{});

  fz_try(raw) { fz_register_document_handlers(raw); }
  fz_catch(raw) {
    std::cerr << "Error registering handlers: " << fz_caught_message(raw)
              << '\n';
    ctx_.reset();
  }
}

PdfSession::~PdfSession() noexcept = default;

// ----- PdfFile
PdfFile::PdfFile(fz_context *ctx, std::string_view path) noexcept : ctx_(ctx) {
  if (!ctx_)
    return;

  // Ensure deleter knows ctx even if open fails
  doc_.deleter().ctx = ctx_;

  fz_document *raw = nullptr;
  std::string pathStr(path);

  fz_try(ctx_) { raw = fz_open_document(ctx_, pathStr.c_str()); }
  fz_catch(ctx_) {
    std::cerr << "Cannot open document: " << fz_caught_message(ctx_) << '\n';
    return;
  }

  if (fz_needs_password(ctx_, raw)) {
    std::cerr << "PDF is password-protected.\n";
    fz_drop_document(ctx_, raw);
    return;
  }

  doc_.reset(raw);
}

PdfFile::~PdfFile() noexcept = default;

int PdfFile::pageCount() const noexcept {
  if (!isValid())
    return 0;
  int count = 0;
  fz_try(ctx_) { count = fz_count_pages(ctx_, doc_.get()); }
  fz_catch(ctx_) {
    std::cerr << "Failed to count pages: " << fz_caught_message(ctx_) << '\n';
  }
  return count;
}
