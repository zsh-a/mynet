#pragma once
#include <netdb.h>
#include <poll.h>
#include <sys/epoll.h>

#include <chrono>
#include <functional>

#include "mynet/common.h"
#include "mynet/event_loop.h"
#include "mynet/resumable.h"

namespace mynet {
class Channel {
 public:
  EventLoop* loop_;
  int fd_{-1};
  uint32_t events_{0};
  uint32_t revents_{0};
  Resumable* resume_read_{nullptr};
  Resumable* resume_write_{nullptr};
  std::chrono::microseconds event_time_{};

  std::function<void()> close_callback_;
  std::function<void()> error_callback_;

 public:
  Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {}
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

  void disable_all() {
    events_ = 0;
    loop_->update_channel(this);
  }

  void handle_event() {
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
      if (close_callback_) close_callback_();
    }

    if (revents_ & (POLLERR | POLLNVAL)) {
      if (error_callback_) error_callback_();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
      if (resume_read_) resume_read_->resume();
    }

    if (revents_ & EPOLLOUT) {
      if (resume_write_) resume_write_->resume();
    }
  }

  struct AwaiterReadEvent {
    bool await_ready() { return false; }
    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> h) {
      if ((channel_->events_ & event_) == 0) {
        channel_->events_ |= event_;
        loop_->update_channel(channel_);
      }
      channel_->set_resume_read(&h.promise());
    }
    void await_resume() {
      // channel_->events_ &= ~event_;
      // loop_->update_channel(channel_);
      channel_->set_resume_read(nullptr);
    }
    int event_;
    Channel* channel_;
    EventLoop* loop_;
  };
  auto read(EventLoop* loop) {
    return AwaiterReadEvent{
        .event_ = kReadeEvent, .channel_ = this, .loop_ = loop};
  }

  struct AwaiterWriteEvent {
    bool await_ready() { return false; }
    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> h) {
      channel_->events_ |= kWriteEvent;
      channel_->set_resume_write(&h.promise());
      loop_->update_channel(channel_);
    }
    void await_resume() {
      channel_->events_ &= ~event_;
      channel_->set_resume_write(nullptr);
      loop_->update_channel(channel_);
    }
    int event_;
    Channel* channel_;
    EventLoop* loop_;
  };

  auto write(EventLoop* loop) {
    return AwaiterWriteEvent{
        .event_ = kWriteEvent, .channel_ = this, .loop_ = loop};
  }

  void register_read(Resumable* task) {
    events_ |= kReadeEvent;
    resume_read_ = task;
    loop_->update_channel(this);
  }

  static constexpr int kReadeEvent = EPOLLIN | EPOLLPRI;
  static constexpr int kWriteEvent = EPOLLOUT;
};

}  // namespace mynet