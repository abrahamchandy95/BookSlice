
#include "db/mongo_repo.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <iostream>
#include <mongocxx/options/index.hpp>
#include <mongocxx/options/update.hpp>

using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;

static void ensure_unique_index(mongocxx::collection &coll) {
  auto keys = document{} << "book_title" << 1 << "chapter" << 1 << "title" << 1
                         << finalize;
  mongocxx::options::index opts;
  opts.unique(true);
  opts.name("unique_section_key_v2");
  try {
    coll.create_index(keys.view(), opts);
  } catch (const std::exception &e) {
    std::cerr << "ensure_unique_index: " << e.what() << '\n';
  }
}

mongocxx::instance &MongoRepository::driver() {
  static mongocxx::instance inst{};
  return inst;
}

MongoRepository::MongoRepository(const MongoConfig &cfg) : cfg_(cfg) {

  (void)driver();
  client_ = mongocxx::client{mongocxx::uri{cfg_.uri}};
  coll_ = client_[cfg_.db][cfg_.coll];
  ensure_ready();
}

MongoRepository::~MongoRepository() = default;

void MongoRepository::ensure_ready() { ensure_unique_index(coll_); }

bool MongoRepository::upsert(const Record &r) {
  mongocxx::options::update opts;
  opts.upsert(true);

  auto filter = document{} << "book_title" << r.book_title << "chapter"
                           << r.chapter << "title" << r.title << finalize;

  auto update =
      document{} << "$set" << open_document << "book_title" << r.book_title
                 << "book_title_src" << r.book_title_src << "book_path"
                 << r.book_path << "chapter_file" << r.chapter_file << "chapter"
                 << r.chapter << "chapter_title" << r.chapter_title
                 << "section_index" << r.section_index << "title" << r.title
                 << "startline" << r.startline << "endline" << r.endline
                 << "content" << r.content << close_document << finalize;

  try {
    auto res = coll_.update_one(filter.view(), update.view(), opts);
    if (!res)
      return false;
    return (res->modified_count() > 0) || bool(res->upserted_id());
  } catch (const std::exception &ex) {
    std::cerr << "Mongo upsert failed for [" << r.chapter_file << " | "
              << r.title << "]: " << ex.what() << '\n';
    return false;
  }
}
