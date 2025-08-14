#pragma once
#include "db/record.hpp"

class Repository {
public:
  virtual ~Repository() = default;

  virtual bool upsert(const Record &rec) = 0;

  // Optional hook to prepare indexes, etc.
  virtual void ensure_ready() {}
};
