#pragma once

namespace mynet {

struct NonCopyable {
 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
  NonCopyable(NonCopyable&&) = default;
  NonCopyable& operator=(NonCopyable&&) = default;

  NonCopyable(NonCopyable&) = delete;
  NonCopyable& operator=(NonCopyable&) = delete;
};

}  // namespace mynet