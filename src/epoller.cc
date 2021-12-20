#include "mynet/poller/epoller.h"

#include "mynet/channel.h"
#include<errno.h>
namespace mynet {

void Epoller::update_channel(Channel* channel) {
  auto it = channels_.find(channel->fd());
  epoll_event event{.events = channel->events(), .data{.ptr = channel}};

  if (it != channels_.end()) {
    if (channel->is_none()) {
      --num_registered;
      ::epoll_ctl(fd_, EPOLL_CTL_DEL, channel->fd(), nullptr);
      channels_.erase(it);
    } else {
      ::epoll_ctl(fd_, EPOLL_CTL_MOD, channel->fd(), &event);
    }
  } else {
    channels_[channel->fd()] = channel;
    ++num_registered;
    if (int ret = ::epoll_ctl(fd_, EPOLL_CTL_ADD, channel->fd(), &event);
        ret != 0) {
      perror("add failed");
      fmt::print("epoll add failed {}\n", fd_);
    }
  }
}

std::vector<Event> Epoller::poll(int timeout) {
  std::vector<epoll_event> events(10);
  int num_fds = epoll_wait(fd_, events.data(), 10, timeout);
  if(num_fds < 0 ){
    perror("epoll");
    return {};
  }

  std::vector<Event> res(num_fds);
  for (int i = 0; i < num_fds; i++) {
    res[i] = Event{.events = events[i].events, .ptr = events[i].data.ptr};
  }
  return res;
}

}  // namespace mynet