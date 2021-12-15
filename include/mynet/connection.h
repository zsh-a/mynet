#pragma once

#include "mynet/task.h"

namespace mynet {

class Connection : NonCopyable {
  int fd_{-1};
  sockaddr_storage sock_info_{};

 public:
  using Buffer = std::vector<char>;
  constexpr static auto BUFFER_SIZE = 4 * 1024;
  Connection() {}
  Connection(Connection&& conn)
      : fd_(std::exchange(conn.fd_, -1)), sock_info_(conn.sock_info_) {}
  Connection(int fd) : fd_(fd) {
    socklen_t addrlen = sizeof(sock_info_);
    getsockname(fd_, reinterpret_cast<sockaddr*>(&sock_info_), &addrlen);
  }

  Task<Buffer> read(ssize_t size = -1) {
    if (size < 0) co_return co_await read_until_eof();
    Event ev{.fd = fd_, .events = EPOLLIN};
    int tot_read = 0;
    Buffer res(size);
    auto& loop = EventLoop::get();
    co_await loop.wait_event(ev);
    tot_read = ::read(fd_, res.data(), size);
    if (tot_read < 0) {
      perror("read error");
      exit(tot_read);
    }
    co_return res;
  }

  Task<Buffer> read_until_eof() {
    Event ev{.fd = fd_, .events = EPOLLIN};
    int tot_read = 0;
    Buffer res;
    auto& loop = EventLoop::get();
    for (int cur_read = 0;;) {
      co_await loop.wait_event(ev);
      Buffer buf(BUFFER_SIZE);
      cur_read = ::read(fd_, buf.data(), BUFFER_SIZE);
      if (cur_read < 0) {
        perror("read error");
        exit(cur_read);
      }
      if (cur_read == 0) break;
      res.insert(res.end(), buf.begin(), buf.end());
      tot_read += cur_read;
    }
    co_return res;
  }

  ~Connection() {
    if (fd_ > 0) ::close(fd_);
  }
};

}  // namespace mynet