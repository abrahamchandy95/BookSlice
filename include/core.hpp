#pragma once
#include <mupdf/fitz/context.h>
#include <mupdf/fitz/document.h>
#include <string>

class MuPdfEnvironment {
public:
  MuPdfEnvironment();
  ~MuPdfEnvironment();

  fz_context *get() const;
  bool isValid() const;

private:
  fz_context *ctx_;
};

class PdfFile {
public:
  PdfFile(fz_context *ctx, const std::string &path);
  ~PdfFile();

  bool isValid() const;
  int pageCount() const;
  fz_document *get() const;
  fz_context *context() const;

private:
  fz_context *ctx_;
  fz_document *doc_;
};
