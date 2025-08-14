#include "db/ingestor.hpp"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "pdf/metadata.hpp"
#include "utils.hpp"

static nlohmann::json read_json_file(const std::filesystem::path &p) {
  std::ifstream is(p);
  if (!is)
    throw std::runtime_error("Failed to open JSON: " + p.string());
  nlohmann::json j;
  is >> j;
  return j;
}

Ingestor::Ingestor(Repository &repo) : repo_(&repo) {}

std::pair<std::size_t, std::size_t>
Ingestor::ingest_chapter_file(const std::filesystem::path &jsonPath,
                              const std::filesystem::path &pdfPath,
                              const BookTitle &book) {
  nlohmann::json j;
  try {
    j = read_json_file(jsonPath);
  } catch (const std::exception &e) {
    std::cerr << "ingest_chapter_file: " << e.what() << '\n';
    return {0, 0};
  }
  if (!j.is_array()) {
    std::cerr << "ingest_chapter_file: JSON is not an array: " << jsonPath
              << '\n';
    return {0, 0};
  }

  const std::string chapterFile = jsonPath.filename().string();
  const std::string chapterStem = jsonPath.stem().string();
  const std::string chapterTitle = Title::extractChapterTitle(chapterFile);

  std::size_t changed = 0;
  const std::size_t total = j.size();

  int section_index = 0;
  for (const auto &item : j) {
    Record rec;
    rec.book_title = book.value;
    rec.book_title_src =
        book.fromMetadata ? ("metadata:" + book.source) : "filename";
    rec.book_path = pdfPath.string();

    rec.chapter_file = chapterFile;
    rec.chapter = chapterStem;
    rec.chapter_title = chapterTitle;

    rec.section_index = section_index;
    rec.title = item.value("title", "");
    rec.startline = item.value("startline", 0);
    rec.endline = item.value("endline", 0);
    rec.content = item.value("content", "");

    if (repo_->upsert(rec))
      ++changed;
    ++section_index;
  }

  std::cout << "DB: upserted/updated " << changed << " / " << total
            << " sections for " << chapterFile << '\n';
  return {changed, total};
}

int Ingestor::ingest_directory(const std::filesystem::path &outDir,
                               const std::filesystem::path &pdfPath,
                               const BookTitle &book) {
  if (!std::filesystem::exists(outDir)) {
    std::cerr << "ingest_directory: output dir not found: " << outDir << "\n";
    return 1;
  }

  std::size_t total_files = 0;
  std::size_t total_changed = 0;
  std::size_t total_sections = 0;

  for (const auto &e : std::filesystem::directory_iterator(outDir)) {
    if (!e.is_regular_file() || e.path().extension() != ".json")
      continue;
    ++total_files;

    auto [changed, total] = ingest_chapter_file(e.path(), pdfPath, book);
    total_changed += changed;
    total_sections += total;
  }

  if (total_files == 0) {
    std::cerr << "ingest_directory: no JSON files found in " << outDir << "\n";
    return 2;
  }

  std::cout << "DB summary: upserted/updated " << total_changed << " / "
            << total_sections << " sections across " << total_files
            << " chapter files.\n";
  return 0;
}
