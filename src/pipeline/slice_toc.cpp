#include "pipeline/slice_toc.hpp"

#include "utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

std::vector<std::string>
SliceToc::readTocLines(const std::filesystem::path &tocPath) const {
  std::ifstream tocIn(tocPath);
  if (!tocIn) {
    std::cerr << "Cannot open " << tocPath << '\n';
    return {};
  }
  std::vector<std::string> tocLines;
  for (std::string ln; std::getline(tocIn, ln);)
    tocLines.push_back(Text::trim(ln));
  return tocLines;
}

std::vector<std::string>
SliceToc::normalize(const std::vector<std::string> &tocLines) const {
  return Text::normalizeLines(tocLines);
}

std::vector<int>
SliceToc::computePositions(const std::vector<std::string> &tocNorm,
                           const std::vector<ChapterMatch> &files) const {
  return indexer_.indexChapters(tocNorm, files);
}

bool SliceToc::isValid(int start, int end) const noexcept {
  if (start < 0 || end < 0 || end <= start)
    return false;
  return (end - start) >= cfg_.minLinesBetweenChapters;
}

int SliceToc::writeSlice(const std::vector<std::string> &tocLines, int start,
                         int end, const std::filesystem::path &outFile) const {
  std::ofstream os(outFile);
  if (!os) {
    std::cerr << "Cannot open " << outFile << " for write\n";
    return 0;
  }
  int written = 0;
  for (int i = start; i < end; ++i) {
    const auto &ln = tocLines[i];
    if (!ln.empty() && !Text::looksLikePageNo(ln)) {
      os << ln << '\n';
      ++written;
    }
  }
  return written;
}

std::size_t SliceToc::run(const std::filesystem::path &tocPath,
                          const std::vector<ChapterMatch> &files) const {
  const auto tocLines = readTocLines(tocPath);
  if (tocLines.empty())
    return 0;

  const auto tocNorm = normalize(tocLines);
  const auto positions = computePositions(tocNorm, files);

  std::filesystem::create_directories(cfg_.outDir);

  std::size_t filesWritten = 0;
  for (size_t idx = 0; idx + 1 < files.size(); ++idx) {
    const int start = positions[idx];
    const int end = positions[idx + 1];
    if (!isValid(start, end))
      continue;

    const std::string stem =
        std::filesystem::path(files[idx].file).stem().string();
    const auto out = std::filesystem::path(cfg_.outDir) / (stem + ".txt");

    const int written = writeSlice(tocLines, start, end, out);
    std::cout << "◆ wrote " << written << " lines → " << out << "  (slice "
              << start << " .. " << (end - 1) << ")\n";
    ++filesWritten;
  }
  return filesWritten;
}
