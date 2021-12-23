#pragma once
#include <netdb.h>
#include <sys/epoll.h>

#include <chrono>

#include "mynet/common.h"
#include "mynet/event_loop.h"
#include "mynet/resumable.h"

namespace mynet {
class Channel {
  int fd_{-1};
  uint32_t events_{0};
  Resumable* resume_read_{nullptr};
  Resumable* resume_write_{nullptr};
  std::chrono::microseconds event_time_{};

 public:
  Channel(int fd) : fd_(fd) {}
  int fd() { return fd_; }
  uint32_t events() { return events_; }
  bool is_none() { return events_ == 0; }
  void set_resume_read(Resumable* resume_read) { resume_read_ = resume_read; }
  void set_resume_write(Resumable* resume_write) {
    resume_write_ = resume_write;
  }
  void set_event_time(std::chrono::microseconds time) { event_time_ = time; }
  auto event_time() { return event_time_; }
  Resumable* resume_read() { return resume_read_; }
  Resumable* resume_write() { return resume_write_; }

  struct AwaiterEvent {
    bool await_ready() { return false; }
    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> h) {
      channel_->events_ |= event_;
      if (event_ == kReadeEvent)
        channel_->set_resume_read(&h.promise());
      else if (event_ == kWriteEvent)
        channel_->set_resume_write(&h.promise());
      loop_->update_channel(channel_);
    }
    void await_resume() {
      channel_->events_ &= ~event_;
      loop_->update_channel(channel_);
    }
    int event_;
    Channel* channel_;
    EventLoop* loop_;
  };
  auto read(EventLoop* loop) {
    return AwaiterEvent{.event_ = kReadeEvent, .channel_ = this, .loop_ = loop};
  }

  auto write(EventLoop* loop) {
    return AwaiterEvent{.event_ = kWriteEvent, .channel_ = this, .loop_ = loop};
  }

  void register_read(EventLoop* loop) {
    events_ |= kReadeEvent;
    loop->update_channel(this);
  }

  static constexpr int kReadeEvent = EPOLLIN | EPOLLPRI;
  static constexpr int kWriteEvent = EPOLLOUT;
};

}  // namespace mynet