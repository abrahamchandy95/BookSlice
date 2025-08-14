#pragma once
#include <string>

struct MongoConfig {
  std::string uri{"mongodb://127.0.0.1:27017"};
  std::string db{"bookslice"};
  std::string coll{"sections"};
};
