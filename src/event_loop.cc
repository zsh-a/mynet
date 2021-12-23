#include "mynet/event_loop.h"

#include <sys/eventfd.h>

#include "mynet/channel.h"
#include<iostream>
namespace mynet {

int create_event_fd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    log::Log(log::Error, "create event fd");
    abort();
  }
  return evtfd;
}

EventLoop::EventLoop()
    : thread_id_(std::this_thread::get_id()),
      wake_fd_(create_event_fd()),
      wake_channel_(new Channel(wake_fd_)) {
  auto now = system_clock::now();
  start_time_ = duration_cast<TimeDuration>(now.time_since_epoch());
  wake_channel_->register_read(this);
}

void EventLoop::run_once() {
  TimeDuration timeout{0};

  if (ready_.size())
    timeout = TimeDuration{0};
  else if (schedule_.size()) {
    timeout = schedule_.front().first - time();
  } else
    timeout = TimeDuration{5000};
  auto events = poller_.poll(timeout.count());
  auto now =
      duration_cast<microseconds>(system_clock::now().time_since_epoch());

  for (const auto& e : events) {
    Channel* channel = reinterpret_cast<Channel*>(e.ptr);
    int revents = e.events;
    // if(revents & EPOLLNVAL){
    //   LOG_WARN << "Channel::handleEvent() POLLNVAL";
    // }

    // if((revents & EPOLLHUP) && !(revents & EPOLLIN)){
    //     if(closeCallback_) closeCallback_();
    // }

    if (revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
      // if(readCallBack_) readCallBack_(receiveTime);
      if (channel->resume_read())
        ready_.emplace(channel->resume_read());
      else {
        // wakeup event
        int64_t i = 0;
        // static int c = 0;
        auto n = ::read(channel->fd(), &i, sizeof(i));
        // fmt::print("{} {}\n",now,++c);
      }
    }

    if (revents & EPOLLOUT) {
      if (channel->resume_write()) ready_.emplace(channel->resume_write());
    }
    channel->set_event_time(now);
  }

  auto end_time = time();
  while (schedule_.size()) {
    auto&& [when, task] = schedule_[0];
    if (when <= end_time) {
      ready_.push(task);
      pop_schedule();
    } else
      break;
  }

  while (ready_.size()) {
    auto handle = ready_.front();
    ready_.pop();
    handle->resume();
  }

  run_pending_tasks();
}

}  // namespace mynet