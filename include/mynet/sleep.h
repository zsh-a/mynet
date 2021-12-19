#pragma once

#include "mynet/common.h"
#include "mynet/event_loop.h"
namespace mynet {
// TODO: replace by timer
struct Sleep {
  bool await_ready() { return false; }
  template <typename Promise>
  void await_suspend(std::coroutine_handle<Promise> h) const noexcept {
    auto& loop = EventLoop::get();
    loop.run_delay(delay_, &h.promise());
  }
  void await_resume() const noexcept {}
  TimeDuration::rep delay_;
};
auto sleep(TimeDuration::rep delay) { return Sleep{delay}; }

}  // namespace mynet