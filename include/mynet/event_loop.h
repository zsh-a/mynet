#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <queue>

#include "mynet/common.h"
#include "mynet/task.h"
namespace mynet {
using namespace std::chrono;
class EventLoop : private NonCopyable {
  using TimeDuration = milliseconds;

  std::queue<Resumable*> ready_;
  using TimerHandle = std::pair<TimeDuration::rep, Resumable*>;
  std::vector<TimerHandle> schedule_;  // heap

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

  void run_at(TimeDuration::rep when, Resumable* task) {
    schedule_.emplace_back(when, task);
    std::push_heap(schedule_.begin(), schedule_.end(), greater{});
  }

  void run_delay(TimeDuration::rep delay, Resumable* task) {
    auto now = system_clock::now();
    schedule_.emplace_back(
        duration_cast<TimeDuration>(now.time_since_epoch()).count() + delay,
        task);
    std::push_heap(schedule_.begin(), schedule_.end(), greater<std::pair<int,int>{});
  }


  void run_until_done(Resumable* routine) {
    ready_.emplace(routine);
    run();
  }

  void run() {
    while (1) {
      run_once();
    }
  }

  void run_once() {
    TimeDuration::rep timeout{0};

    while (schedule_.size()) {
      auto now = system_clock::now();
      auto time = duration_cast<TimeDuration>(now.time_since_epoch()).count();
      auto&& [when, task] = schedule_[0];
      if (when >= time) {
        while (!task->done()) task->resume();
        pop_heap(schedule_.begin(), schedule_.end());
      } else
        break;

      //   timeout = when;
    }
    while (ready_.size()) {
      auto handle = std::move(ready_.front());
      ready_.pop();
      while (!handle->done()) handle->resume();
    }
  }
};

}  // namespace mynet