#pragma once

#include "mynet/common.h"
#include "mynet/event_loop.h"
namespace mynet {
// TODO: replace by timer
template <typename Duration>
struct Sleep {
  bool await_ready() { return false; }
  template <typename Promise>
  void await_suspend(std::coroutine_handle<Promise> h) const noexcept {
    loop_->run_delay(delay_, &h.promise());
  }
  void await_resume() const noexcept {}
  EventLoop* loop_;
  Duration delay_;
};
template <typename Rep, typename Period>
auto sleep(EventLoop* loop, std::chrono::duration<Rep, Period> delay) {
  return Sleep{loop, delay};
}

}  // namespace mynet