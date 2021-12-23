#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

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
  std::thread::id thread_id_;
  std::queue<Resumable*> ready_;
  using TimerHandle = std::pair<TimeDuration, Resumable*>;
  std::vector<TimerHandle> schedule_;  // heap
  Poller poller_;

  int wake_fd_{-1};
  std::unique_ptr<Channel> wake_channel_;

  mutable std::mutex mutex_;
  std::vector<Resumable*> pending_tasks_;  // protected by mutex_

  bool do_pending_tasks_{false};

  void run_pending_tasks() {
    do_pending_tasks_ = true;
    std::vector<Resumable*> tasks;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      tasks.swap(pending_tasks_);
    }
    for (auto taks : tasks) {
      taks->resume();
    }
    do_pending_tasks_ = false;
  }

  bool is_in_loop_thread() { return std::this_thread::get_id() == thread_id_; }

 public:
  TimeDuration start_time_;

  EventLoop();

  // static EventLoop& get() {
  //   static EventLoop loop;
  //   return loop;
  // }
  template <typename Task>
  auto create_task(Task&& task) {
    task.handle_.promise().loop_ = this;
    run_immediately(task.get_resumable());
    return std::forward<Task>(task);
  }

  template <typename Task>
  auto&& queue_in_loop(Task&& task){
    run_in_loop(task.get_resumable());
    return std::forward<Task>(task);
  }

  void run_in_loop(Resumable* task) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      pending_tasks_.push_back(task);
    }
    if (!is_in_loop_thread() || do_pending_tasks_) {
      wakeup();
    }
  }

  void update_channel(Channel* channel) { poller_.update_channel(channel); }

  TimeDuration time() {
    auto now = system_clock::now();
    return duration_cast<TimeDuration>(now.time_since_epoch()) - start_time_;
  }

  void run_immediately(Resumable* task) { ready_.emplace(task); }

  template <typename Rep, typename Period>
  void run_at(std::chrono::duration<Rep, Period> when, Resumable* task) {
    schedule_.emplace_back(duration_cast<TimeDuration>(when), task);
    std::push_heap(schedule_.begin(), schedule_.end(), std::greater{});
  }

  template <typename Rep, typename Period>
  void run_delay(std::chrono::duration<Rep, Period> delay, Resumable* task) {
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

  void wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wake_fd_, &one, sizeof one);
    if (n != sizeof one) {
      log::Log(log::Error, "wakeup return {}", n);
    }
  }

  void pop_schedule() {
    pop_heap(schedule_.begin(), schedule_.end());
    schedule_.pop_back();
  }
  void run_once();
};

}  // namespace mynet