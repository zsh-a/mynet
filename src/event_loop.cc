#include "mynet/event_loop.h"

#include <sys/eventfd.h>

#include <iostream>

#include "mynet/channel.h"
#include "mynet/task.h"
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
      wake_channel_(new Channel(this, wake_fd_)) {
  auto now = system_clock::now();
  start_time_ = duration_cast<TimeDuration>(now.time_since_epoch());

  wake_channel_->register_read(new Func{
    [this](){
      int64_t i = 0;
      ::read(wake_fd_,&i,sizeof i);
    }
  });

  log::Log(log::Info, "wake up fd : {}", wake_fd_);
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
  // fmt::print("~~~~~~~{}\n",events.size());
  // if(events.size() > 0){
  //   fmt::print("eve : {}\n",events[0].events);
  // }

  for (const auto& e : events) {
    Channel* channel = reinterpret_cast<Channel*>(e.ptr);
    channel->revents_ = e.events;
    channel->set_event_time(now);
    channel->handle_event();
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