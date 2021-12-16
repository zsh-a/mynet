#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

#include "event.h"
#include "fmt/format.h"
namespace mynet {

class Epoller {

  public:
  int fd_;
  int num_registered{0};

 public:
  Epoller() : fd_(epoll_create1(0)) {
    if (fd_ == -1) {
      perror("epoll_create1");
      exit(EXIT_FAILURE);
    }
  }

  bool is_stop() { return num_registered <= 0; }

  std::vector<Event> poll(int timeout) {
    std::vector<epoll_event> events(10);
    int num_fds = epoll_wait(fd_, events.data(), 10, timeout);
    std::vector<Event> res(num_fds);
    for (int i = 0; i < num_fds; i++) {
      res[i] = Event{.ptr = events[i].data.ptr};
    }
    return res;
  }

  void register_event(const Event& event) {
    epoll_event ev{.events = event.events, .data{.ptr = event.ptr}};
    if (int ret = epoll_ctl(fd_, EPOLL_CTL_ADD, event.fd, &ev); ret == 0) {
      ++num_registered;
    } else {
      perror("add failed");
      fmt::print("epoll add failed {} {} {}\n",event.fd, event.events,fd_);
    }
  }

  void remove_event(const Event& event) {
    epoll_event ev{.events = event.events, .data{.ptr = event.ptr}};
    if (int ret = epoll_ctl(fd_, EPOLL_CTL_DEL, event.fd, &ev); ret == 0) {
      --num_registered;
    } else {
      fmt::print("epoll del failed {}\n", ret);
    }
  }

  ~Epoller() { close(fd_); }
};

}  // namespace mynet