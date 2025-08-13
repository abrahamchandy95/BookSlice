#include "pdf/metadata.hpp"

#include <cctype>
#include <iostream>
#include <mupdf/fitz.h>

#include "utils.hpp"

namespace {

std::string toTitleCase(std::string s) {
  bool newWord = true;
  for (char &c : s) {
    unsigned char uc = static_cast<unsigned char>(c);
    if (std::isspace(uc)) {
      newWord = true;
    } else if (newWord) {
      c = static_cast<char>(std::toupper(uc));
      newWord = false;
    } else {
      c = static_cast<char>(std::tolower(uc));
    }
  }
  return s;
}

std::string lookup_meta_string(fz_context *ctx, fz_document *doc,
                               const char *key) {
  if (!ctx || !doc)
    return {};
  char buf[1024] = {0};

  fz_try(ctx) {
    fz_lookup_metadata(ctx, doc, key, buf, static_cast<int>(sizeof(buf)));
  }
  fz_catch(ctx) {
    std::cerr << "MuPDF metadata lookup failed for key '" << key
              << "': " << fz_caught_message(ctx) << '\n';
    return {};
  }

  if (buf[0] == '\0')
    return {};
  std::string out(buf);
  out = Text::collapseWhitespace(out);
  out = Text::trim(out);
  return out;
}

bool isInvalid(const std::string &s) {
  if (s.empty())
    return true;

  // Lowercased string
  std::string lower = s;
  for (char &c : lower) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  if (lower == "untitled" || lower == "unknown" || lower == "null")
    return true;

  const bool hasSpace = (s.find(' ') != std::string::npos);

  int digits = 0, alnum = 0;
  for (unsigned char c : s) {
    if (std::isalnum(c)) {
      ++alnum;
      if (std::isdigit(c))
        ++digits;
    }
  }

  if (!hasSpace && alnum > 0 && (static_cast<double>(digits) / alnum) >= 0.7)
    return true;

  if (!hasSpace && s.size() <= 6)
    return true;

  return false;
}

} // namespace

std::string pdfDocumentTitle(fz_context *ctx, fz_document *doc) {
  if (auto t = lookup_meta_string(ctx, doc, "info:Title"); !t.empty())
    return t;
  if (auto t = lookup_meta_string(ctx, doc, "title"); !t.empty())
    return t;
  if (auto t = lookup_meta_string(ctx, doc, "Title"); !t.empty())
    return t;
  return {};
}

BookTitle getBookTitle(fz_context *ctx, fz_document *doc,
                       const std::filesystem::path &pdfPath) {
  struct Key {
    const char *k;
  };
  static constexpr Key keys[] = {{"info:Title"}, {"title"}, {"Title"}};
  for (const auto &key : keys) {
    if (auto t = lookup_meta_string(ctx, doc, key.k); !t.empty()) {
      if (!isInvalid(t)) {
        return BookTitle{t, true, key.k};
      }
      std::cerr << "PDF metadata title looks invalid: '" << t
                << "' — falling back to filename.\n";
      break;
    }
  }

  std::string base = pdfPath.stem().string(); // drop extension
  for (char &c : base) {
    if (c == '_' || c == '-' || c == '.')
      c = ' ';
  }
  base = Text::collapseWhitespace(base);
  base = Text::trim(base);
  base = toTitleCase(base);

  return BookTitle{base, false, "filename"};
}
