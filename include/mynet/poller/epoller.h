#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

#include "event.h"
namespace mynet {

class Epoller {
  int fd_;

 public:
  Epoller() : fd_(epoll_create1(0)) {
    if (fd_ == -1) {
      perror("epoll_create1");
      exit(EXIT_FAILURE);
    }
  }

  std::vector<epoll_event> poll(int timeout) {
    std::vector<epoll_event> events(10);
    int num_fds = epoll_wait(fd_, events.data(), 10, timeout);
    return events;
  }

  ~Epoller() { close(fd_); }
};

}  // namespace mynet