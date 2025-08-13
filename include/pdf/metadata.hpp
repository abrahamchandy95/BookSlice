#pragma once
#include <filesystem>
#include <mupdf/fitz.h>
#include <string>

struct BookTitle {
  std::string value;
  bool fromMetadata{false};
  std::string source;
};

std::string pdfDocumentTitle(fz_context *ctx, fz_document *doc);

BookTitle getBookTitle(fz_context *ctx, fz_document *doc,
                       const std::filesystem::path &pdfPath);

inline std::string inferBookTitle(fz_context *ctx, fz_document *doc,
                                  const std::filesystem::path &pdfPath) {
  return getBookTitle(ctx, doc, pdfPath).value;
}
