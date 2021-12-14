#pragma once
#include <functional>
#include<chrono>
#include "mynet/common.h"
namespace mynet {
using TimerHandler = std::function<void()>;

// std::chrono::duration<Rep, Period>;
class Timer : private NonCopyable {
  static uint64_t num_created_;
  TimerHandler handler_;

 public:
};

}  // namespace mynet