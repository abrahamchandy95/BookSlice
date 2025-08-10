#pragma once
#include "handle.hpp"
#include <mupdf/fitz.h>
#include <string_view>

struct FzContextDrop {
  void operator()(fz_context *ctx) const noexcept;
};

struct FzDocumentDrop {
  fz_context *ctx{};
  void operator()(fz_document *doc) const noexcept;
};
// ── Session = engine lifetime (replaces MuPdfEnvironment) ───────────────────

class PdfSession {
public:
  PdfSession() noexcept;
  ~PdfSession() noexcept;
  bool isValid() const noexcept { return static_cast<bool>(ctx_); }
  fz_context *ctx() const noexcept { return ctx_.get(); }

private:
  Handle<fz_context, FzContextDrop> ctx_{};
};

// File = opened PDF tied to a session
class PdfFile {
public:
  PdfFile(fz_context *ctx, std::string_view path) noexcept;
  ~PdfFile() noexcept; // out-of-line dtor
  bool isValid() const noexcept { return static_cast<bool>(doc_); }
  int pageCount() const noexcept;

  fz_document *doc() const noexcept { return doc_.get(); }
  fz_context *ctx() const noexcept { return ctx_; }

private:
  fz_context *ctx_ = nullptr; // not owned
  Handle<fz_document, FzDocumentDrop> doc_{nullptr, FzDocumentDrop{nullptr}};
};
