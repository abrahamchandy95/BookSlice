#pragma once
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "db/mongo_conf.hpp"
#include "db/repository.hpp"

class MongoRepository : public Repository {
public:
  explicit MongoRepository(const MongoConfig &cfg = {});
  ~MongoRepository() override;

  void ensure_ready() override;
  bool upsert(const Record &rec) override;

private:
  static mongocxx::instance &driver();

  MongoConfig cfg_;
  mongocxx::client client_;
  mongocxx::collection coll_;
};
