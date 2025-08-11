#pragma once
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

struct Outline;

struct Text {
  static std::string toLower(const std::string &s);
  static bool isSpace(char c) noexcept;
  static std::string trim(const std::string &s);
  static std::string collapseWhitespace(const std::string &s);
  static std::string normalizeStr(std::string s);
  static bool looksLikePageNo(const std::string &s);
  static std::vector<std::string>
  normalizeLines(const std::vector<std::string> &lines);
};

struct Title {
  static void replaceAll(std::string &s, const std::string &from,
                         const std::string &to);
  static std::string stripLeadingChapterTag(const std::string &s);
  static std::string slugify(const std::string &s);
  static bool looksLikeTocName(const std::string &filename);
  static std::string extractChapterTitle(const std::string &path);
};

struct FileIO {
  static std::vector<std::string> readLines(const std::filesystem::path &p);
  static void writeJson(const std::filesystem::path &outPath,
                        const nlohmann::json &j);
};

struct OutlineView {
  static void print(const std::vector<Outline> &outline, int totalPages);
};
