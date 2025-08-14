#pragma once
#include <filesystem>
#include <utility>

#include "db/repository.hpp"
#include "pdf/metadata.hpp"

class Ingestor {
public:
  explicit Ingestor(Repository &repo);

  std::pair<std::size_t, std::size_t>
  ingest_chapter_file(const std::filesystem::path &jsonPath,
                      const std::filesystem::path &pdfPath,
                      const BookTitle &book);

  int ingest_directory(const std::filesystem::path &outDir,
                       const std::filesystem::path &pdfPath,
                       const BookTitle &book);

private:
  Repository *repo_;
};
