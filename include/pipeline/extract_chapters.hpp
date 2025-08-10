#pragma once
#include "pdf/session.hpp"
#include "types.hpp"
#include <vector>

bool extractChapters(const PdfSession &session, const PdfFile &pdf,
                     int &totalPages, std::vector<ChapterInfo> &chapters,
                     bool topLevelOnly = true);
