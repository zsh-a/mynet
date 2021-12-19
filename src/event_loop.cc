#include "mynet/event_loop.h"

#include "mynet/channel.h"
namespace mynet {

void EventLoop::run_once() {
  TimeDuration timeout{0};

  if (ready_.size())
    timeout = TimeDuration{0};
  else if (schedule_.size()) {
    timeout = schedule_.front().first - time();
  } else
    timeout = TimeDuration{5000};
  auto events = poller_.poll(timeout.count());
  auto now = duration_cast<microseconds>(system_clock::now().time_since_epoch());

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
      ready_.emplace(channel->resume_read());
    }

    if (revents & EPOLLOUT) {
      ready_.emplace(channel->resume_write());
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
  
}

}  // namespace mynet