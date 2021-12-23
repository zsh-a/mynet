#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>

#include "mynet/event_loop.h"
namespace mynet {

class EventLoopThread {
  EventLoop* loop_{nullptr};
  std::jthread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;

 public:
  EventLoopThread() {}

  EventLoop* start() {
    thread_ = std::jthread([&]() { thread_func(); });
    {
      std::unique_lock<std::mutex> lk(mutex_);
      cv_.wait(lk, [&] { return loop_ != nullptr; });
    }
    return loop_;
  }

  void thread_func() {
    EventLoop loop;
    {
      std::lock_guard<std::mutex> lk(mutex_);
      loop_ = &loop;
    }
    cv_.notify_one();
    loop.run();
  }
};

}  // namespace mynet