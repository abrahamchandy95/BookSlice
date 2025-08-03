
#include <iostream>
#include <mupdf/fitz.h>

#include "core.hpp"

MuPdfEnvironment::MuPdfEnvironment() {
  ctx_ = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
  if (!ctx_) {
    std::cerr << "Failed to create MuPDF context.\n";
    return;
  }
  fz_try(ctx_) { fz_register_document_handlers(ctx_); }
  fz_catch(ctx_) {
    std::cerr << "Error registering handlers: " << fz_caught_message(ctx_)
              << '\n';
    fz_drop_context(ctx_);
    ctx_ = nullptr;
  }
}

MuPdfEnvironment::~MuPdfEnvironment() {
  if (ctx_)
    fz_drop_context(ctx_);
}

fz_context *MuPdfEnvironment::get() const { return ctx_; }
bool MuPdfEnvironment::isValid() const { return ctx_ != nullptr; }

PdfFile::PdfFile(fz_context *ctx, const std::string &path)
    : ctx_(ctx), doc_(nullptr) {
  if (!ctx_)
    return;

  fz_try(ctx_) { doc_ = fz_open_document(ctx_, path.c_str()); }
  fz_catch(ctx_) {
    std::cerr << "Cannot open document: " << fz_caught_message(ctx_) << '\n';
    return;
  }

  if (fz_needs_password(ctx_, doc_)) {
    std::cerr << "PDF is password-protected.\n";
    fz_drop_document(ctx_, doc_);
    doc_ = nullptr;
  }
}

PdfFile::~PdfFile() {
  if (doc_)
    fz_drop_document(ctx_, doc_);
}

bool PdfFile::isValid() const { return doc_ != nullptr; }

int PdfFile::pageCount() const {
  if (!isValid())
    return 0;
  int count{};
  fz_try(ctx_) { count = fz_count_pages(ctx_, doc_); }
  fz_catch(ctx_) {
    std::cerr << "Failed to count pages: " << fz_caught_message(ctx_) << '\n';
  }
  return count;
}

fz_document *PdfFile::get() const { return doc_; }
fz_context *PdfFile::context() const { return ctx_; }
