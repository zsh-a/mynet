#pragma once
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "mynet/connection.h"
#include "mynet/task.h"
#include "mynet/sleep.h"
namespace mynet {

namespace internel {
Task<bool> connect(EventLoop* loop, int fd, const sockaddr* addr,
                   socklen_t len) noexcept {
  int ret = ::connect(fd, addr, len);
  if (ret == 0) co_return true;
  if (ret < 0 && errno != EINPROGRESS) {
    perror("exception happened");
    exit(errno);
  }
  Channel channel(loop, fd);
  co_await channel.write(loop);
  int result{0};
  socklen_t result_len = sizeof(result);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
    co_return false;
  }
  co_return result == 0;
}

struct AddressInfo {
  AddressInfo(addrinfo* info) : info_(info) {}
  ~AddressInfo() { freeaddrinfo(info_); }
  addrinfo* info_{nullptr};
};
}  // namespace internel

Task<Connection> open_connection(EventLoop* loop, std::string_view ip,
                                 int port) {
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
  auto p = server_info;
  fd = -1;
  int cnt = 2;
  for (;;) {
    if ((fd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK,
                     p->ai_protocol)) == -1) {
      log::Log(log::Error, "create socket");
      exit(errno);
    }

    if ((co_await internel::connect(loop, fd, p->ai_addr, p->ai_addrlen)
             .run_in(loop))) {
      break;
    }
    ::close(fd);
    fmt::print("connect to {}:{} failed, waiting {}s. retrying...\n", ip,
               port_str, cnt);
    co_await mynet::sleep(loop, std::chrono::seconds(cnt));
    cnt *= 2;
  }

  if (fd == -1) {
    perror("socket connect failed");
    exit(fd);
  }

  co_return Connection{loop, fd, fmt::format("client:{}:{}", ip, port)};
}

}  // namespace mynet