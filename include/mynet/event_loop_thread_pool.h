#pragma once
#include <iostream>
#include <memory>
#include <thread>

#include "mynet/common.h"
#include "mynet/event_loop_thread.h"
namespace mynet {
class EventLoop;
class EventLoopThreadPool {
  EventLoop* base_;
  int num_threads_{0};
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;

  int next_{0};

 public:
  EventLoopThreadPool(EventLoop* base, int num_thread = 0)
      : base_(base), num_threads_(num_thread) {
    for (int i = 0; i < num_threads_; i++) {
      threads_.emplace_back(std::make_unique<EventLoopThread>());
      loops_.push_back(threads_[i]->start());
    }
  }
  EventLoop* get_next_loop() {
    EventLoop* loop = base_;
    if (loops_.size()) {
      loop = loops_[next_];
      next_ = (next_ + 1) % num_threads_;
    }
    return loop;
  }
};

}  // namespace mynet