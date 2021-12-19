#include "mynet/event_loop.h"
#include "mynet/channel.h"
namespace mynet {

void EventLoop::run_once() {
  // TimeDuration::rep timeout{0};

  int timeout = -1;
  if (ready_.size())
    timeout = 0;
  else
    timeout = 5000;
  auto events = poller_.poll(timeout);
  // fmt::print("{} events happened {}
  // \n",events.size(),system_clock::now().time_since_epoch().count());

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
  }
  while (ready_.size()) {
    auto handle = ready_.front();
    ready_.pop();
    handle->resume();
  }

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
}

}  // namespace mynet