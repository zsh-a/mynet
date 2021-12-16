#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <queue>

#include "mynet/common.h"
#include "mynet/poller/epoller.h"
#include "mynet/resumable.h"
#include "fmt/format.h"
namespace mynet {
using namespace std::chrono;
using TimeDuration = milliseconds;
using Poller = Epoller;
class EventLoop : private NonCopyable {
  std::queue<Resumable*> ready_;
  using TimerHandle = std::pair<TimeDuration::rep, Resumable*>;
  std::vector<TimerHandle> schedule_;  // heap
  Poller poller_;

 public:
  TimeDuration::rep start_time_;

  EventLoop() {
    auto now = system_clock::now();
    start_time_ = duration_cast<TimeDuration>(now.time_since_epoch()).count();
  }

  static EventLoop& get() {
    static EventLoop loop;
    return loop;
  }

  TimeDuration::rep time() {
    auto now = system_clock::now();
    return duration_cast<TimeDuration>(now.time_since_epoch()).count() -
           start_time_;
  }

  void run_immediately(Resumable* task){
    ready_.emplace(task);
  }

  void run_at(TimeDuration::rep when, Resumable* task) {
    schedule_.emplace_back(when, task);
    std::push_heap(schedule_.begin(), schedule_.end(), std::greater{});
  }

  void run_delay(TimeDuration::rep delay, Resumable* task) {
    auto now = system_clock::now();
    run_at(duration_cast<TimeDuration>(now.time_since_epoch()).count() + delay,
           task);
  }

  void run_until_done(Resumable* routine) {
    ready_.push(routine);
    run();
  }

  void run() {
    while (schedule_.size() || ready_.size() || !poller_.is_stop()) {
      run_once();
      // auto now = system_clock::now();
      // auto time = duration_cast<TimeDuration>(now.time_since_epoch()).count();
      // if (time - start_time_ >= 5000) break;
    }
  }

  void pop_schedule() {
    pop_heap(schedule_.begin(), schedule_.end());
    schedule_.pop_back();
  }
  void run_once() {
    // TimeDuration::rep timeout{0};

    int timeout = -1;
    if(ready_.size()) timeout = 0;
    else timeout = 5000;
    auto events = poller_.poll(timeout);
    // fmt::print("{} events happened {} \n",events.size(),system_clock::now().time_since_epoch().count());

    for(const auto& e : events){
      ready_.push(reinterpret_cast<Resumable*>(e.ptr));
    }
    while (ready_.size()) {
      auto handle = ready_.front();
      ready_.pop();
      handle->resume();
    }

    while (schedule_.size()) {
      auto now = system_clock::now();
      auto time = duration_cast<TimeDuration>(now.time_since_epoch()).count();
      auto&& [when, task] = schedule_[0];
      if (time >= when) {
        while (!task->done()) task->resume();
        pop_schedule();
      } else
        break;

      //   timeout = when;
    }
  }

  struct AwaiterEvent{
    bool await_ready() { return false; }
    template<typename Promise>
    void await_suspend(std::coroutine_handle<Promise> h) {
      event_.ptr = &h.promise();
      poller_.register_event(event_);
    }
    constexpr void await_resume() const noexcept {}
    void await_resume() { poller_.remove_event(event_); }
    Event event_;
    Poller& poller_;
  };

  auto wait_event(const Event& event){
    return AwaiterEvent{event,poller_};
  }

};

}  // namespace mynet