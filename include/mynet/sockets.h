#pragma once
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "mynet/task.h"
#include "mynet/connection.h"
namespace mynet {

namespace internel {
Task<bool> connect(int fd, const sockaddr* addr, socklen_t len) noexcept {
  int ret = ::connect(fd, addr, len);
  if (ret == 0) co_return true;
  if (ret < 0 && errno != EINPROGRESS) {
    perror("exception happened");
    exit(EINPROGRESS);
  }

  auto& loop = EventLoop::get();
  Event ev{.fd = fd, .events = EPOLLOUT};
  co_await loop.wait_event(ev);
}

struct AddressInfo {
  AddressInfo(addrinfo* info) : info_(info) {}
  ~AddressInfo() { freeaddrinfo(info_); }
  addrinfo* info_{nullptr};
};
}  // namespace internel

Task<Connection> open_connection(std::string_view ip, int port) {
  addrinfo hints{
      .ai_family = AF_UNSPEC,
      .ai_socktype = SOCK_STREAM,
  };

  addrinfo* server_info{nullptr};
  auto port_str = std::to_string(port);
  if (int ret = getaddrinfo(ip.data(), port_str.c_str(), &hints, &server_info);
      ret != 0) {
    perror("getaddrinfo");
    exit(ret);
  }
  auto address = internel::AddressInfo{server_info};
  int fd = -1;
  for (auto p = server_info; p != nullptr; p = p->ai_next) {
    fd = -1;
    if ((fd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK,
                     p->ai_protocol)) == 1)
      continue;
    if(!(co_await internel::connect(fd,p->ai_addr,p->ai_addrlen))){
        fmt::print("connect to {}:{} failed, retrying\n",p->ai_addr->sa_data,port_str);
        close(fd);
        continue;
    }
  }
  if(fd == -1){
      perror("socket connect failed");
      exit(fd);
  }
  co_return Connection{fd};
}

}  // namespace mynet