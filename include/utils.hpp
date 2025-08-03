#pragma once
#include "extract.hpp"
#include <string>

struct Outline;
std::string trim(const std::string &s);
std::string slugify(const std::string &title);
void printOutline(const std::vector<Outline> &outline, int totalPages);
