#pragma once
#include <utility>

template <class T, class Deleter> class Handle {
public:
  Handle() noexcept : ptr_(nullptr), del_{} {}
  Handle(T *ptr, Deleter del) noexcept : ptr_(ptr), del_(del) {}
  ~Handle() noexcept { reset(); }

  Handle(const Handle &) = delete;
  Handle &operator=(const Handle &) = delete;

  Handle(Handle &&src) noexcept
      : ptr_(std::exchange(src.ptr_, nullptr)), del_(std::move(src.del_)) {}

  Handle &operator=(Handle &&rhs) noexcept {
    if (this != &rhs) {
      reset();
      ptr_ = std::exchange(rhs.ptr_, nullptr);
      del_ = std::move(rhs.del_);
    }
    return *this;
  }
  T *get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  T *release() noexcept { return std::exchange(ptr_, nullptr); }

  void reset(T *p = nullptr) noexcept {
    if (ptr_)
      del_(ptr_);
    ptr_ = p;
  }
  Deleter &deleter() noexcept { return del_; }
  Deleter const &deleter() const noexcept { return del_; }

private:
  T *ptr_ = nullptr;
  [[no_unique_address]] Deleter del_{};
};
