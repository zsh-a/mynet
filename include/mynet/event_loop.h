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
class Channel;
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

  void update_channel(Channel* channel){
    poller_.update_channel(channel);
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
    }
  }

  void pop_schedule() {
    pop_heap(schedule_.begin(), schedule_.end());
    schedule_.pop_back();
  }
  void run_once();

};

}  // namespace mynet