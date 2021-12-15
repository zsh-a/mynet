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
  std::queue<std::unique_ptr<Resumable>> ready_;
  using TimerHandle = std::pair<TimeDuration::rep, std::unique_ptr<Resumable>>;
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

  void run_immediately(std::unique_ptr<Resumable> task){
    ready_.emplace(std::move(task));
  }

  void run_at(TimeDuration::rep when, std::unique_ptr<Resumable> task) {
    schedule_.emplace_back(when, std::move(task));
    std::push_heap(schedule_.begin(), schedule_.end(), std::greater{});
  }

  void run_delay(TimeDuration::rep delay, std::unique_ptr<Resumable> task) {
    auto now = system_clock::now();
    run_at(duration_cast<TimeDuration>(now.time_since_epoch()).count() + delay,
           std::move(task));
  }

  void run_until_done(std::unique_ptr<Resumable> routine) {
    ready_.push(std::move(routine));
    run();
  }

  void run() {
    while (schedule_.size() || ready_.size()) {
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
    TimeDuration::rep timeout{0};

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

    auto events = poller_.poll(5000);

    for(const auto& e : events){
      ready_.push(std::unique_ptr<Resumable>(reinterpret_cast<Resumable*>(e.ptr)));
      fmt::print("event {} \n",e.ptr);
    }

    while (ready_.size()) {
      auto handle = std::move(ready_.front());
      ready_.pop();
      handle->resume();
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
    // R await_resume() { return self_.promise().value_; }
    Event event_;
    Poller poller_;
  };

  auto wait_event(const Event& event){
    return AwaiterEvent{event,poller_};
  }

};

}  // namespace mynet