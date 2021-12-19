#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <map>
#include <vector>

#include "event.h"
#include "fmt/format.h"
namespace mynet {
class Channel;
class Epoller {
 public:
  int fd_;
  int num_registered{0};
  std::map<int, Channel*> channels_;

 public:
  Epoller() : fd_(epoll_create1(0)) {
    if (fd_ == -1) {
      perror("epoll_create1");
      exit(EXIT_FAILURE);
    }
  }

  bool is_stop() { return num_registered <= 0; }

  std::vector<Event> poll(int timeout);

  void update_channel(Channel* channel);

  ~Epoller() { close(fd_); }
};

}  // namespace mynet