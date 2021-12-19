#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <queue>

#include "fmt/format.h"
#include "mynet/common.h"
#include "mynet/poller/epoller.h"
#include "mynet/resumable.h"
namespace mynet {
class Channel;
using namespace std::chrono;
using TimeDuration = milliseconds;
using Poller = Epoller;
class EventLoop : private NonCopyable {
  std::queue<Resumable*> ready_;
  using TimerHandle = std::pair<TimeDuration, Resumable*>;
  std::vector<TimerHandle> schedule_;  // heap
  Poller poller_;

 public:
  TimeDuration start_time_;

  EventLoop() {
    auto now = system_clock::now();
    start_time_ = duration_cast<TimeDuration>(now.time_since_epoch());
  }

  static EventLoop& get() {
    static EventLoop loop;
    return loop;
  }

  void update_channel(Channel* channel) { poller_.update_channel(channel); }

  TimeDuration time() {
    auto now = system_clock::now();
    return duration_cast<TimeDuration>(now.time_since_epoch()) -
           start_time_;
  }

  void run_immediately(Resumable* task) { ready_.emplace(task); }

  template <typename Rep, typename Period>
  void run_at(std::chrono::duration<Rep, Period> when, Resumable* task) {
    schedule_.emplace_back(duration_cast<TimeDuration>(when), task);
    std::push_heap(schedule_.begin(), schedule_.end(), std::greater{});
  }

  template <typename Rep, typename Period>
  void run_delay(std::chrono::duration<Rep, Period> delay, Resumable* task) {
    auto now = system_clock::now();
    run_at(time() + duration_cast<TimeDuration>(delay), task);
  }

  void run_until_done(Resumable* routine) {
    ready_.push(routine);
    run();
  }

  void run() {
    while (schedule_.size() || ready_.size() || !poller_.is_stop()) {
      run_once();
    }
  }

  void pop_schedule() {
    pop_heap(schedule_.begin(), schedule_.end());
    schedule_.pop_back();
  }
  void run_once();
};

}  // namespace mynet