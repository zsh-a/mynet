#pragma once

#include "mynet/event_loop.h"
#include"mynet/common.h"
namespace mynet {

auto sleep(TimeDuration::rep delay) {
  struct Sleep {
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept{
        auto& loop = EventLoop::get();
        loop.run_delay(delay_,std::make_unique<CoResumable>(h));
    }
    void await_resume() const noexcept {}
    TimeDuration::rep delay_;
  };
  return Sleep{delay};
}

}  // namespace mynet