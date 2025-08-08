#pragma once
#include "extract.hpp"
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string>

struct Outline;
std::string toLower(const std::string &s);
bool isSpace(char c);
std::string trim(const std::string &s);
std::string collapseWhitespace(const std::string &s);
std::string slugify(const std::string &title);
void printOutline(const std::vector<Outline> &outline, int totalPages);
std::string normalize_str(std::string s);
bool fuzzy_contains(const std::string &hay, const std::string &needle);
std::vector<std::string> readLines(const std::filesystem::path &p);
void writeJson(const std::filesystem::path &outPath, const nlohmann::json &j);
