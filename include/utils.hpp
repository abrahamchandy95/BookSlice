#pragma once
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct Outline;

struct Text {
  static std::string toLower(const std::string &s);
  static bool hasLetters(std::string_view s) noexcept;
  static double upperRatio(std::string_view s) noexcept;
  static bool isSpace(char c) noexcept;
  static std::string trim(const std::string &s);
  static std::string collapseWhitespace(std::string_view s);
  static std::string normalizeStr(std::string s);
  static bool contains(std::string_view hay, std::string_view needle);
  static bool looksLikePageNo(const std::string &s);
  static std::vector<std::string>
  normalizeLines(const std::vector<std::string> &lines);
};

struct Title {
  static void replaceAll(std::string &s, const std::string &from,
                         const std::string &to);
  static std::string stripLeadingChapterTag(const std::string &s);
  static std::string stripLeadingThe(std::string s);
  static std::string slugify(const std::string &s);
  static std::filesystem::path findToc(const std::filesystem::path &dir);
  static bool looksLikeTocName(const std::string &filename);
  static bool hasChapterName(std::string_view s,
                             const std::string &chapterTitle);
  static bool isWordBannedGiven(std::string_view s,
                                std::span<const std::string_view> words);
  static bool hasBannedWord(std::string_view lower);
  static bool hasWeirdChars(std::string_view s);
  static bool isNoisy(const std::string &s, const std::string &chapterTitle);
  static std::string extractChapterTitle(const std::string &path);
  static bool isSubtitleMatch(std::string_view tocLine,
                              std::string_view chapterLine);
};

struct FileIO {
  static std::vector<std::string> readLines(const std::filesystem::path &p);
  static void writeJson(const std::filesystem::path &outPath,
                        const nlohmann::json &j);
  static std::vector<std::filesystem::path>
  listChapters(const std::filesystem::path &dir, std::string_view ext);
};

struct OutlineView {
  static void print(const std::vector<Outline> &outline, int totalPages);
};
