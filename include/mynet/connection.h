#pragma once

#include <netinet/tcp.h>
#include <sys/socket.h>

#include "mynet/channel.h"
#include "mynet/task.h"
namespace mynet {

class Connection : NonCopyable {
 public:
  EventLoop* loop_{nullptr};
  int fd_{-1};
  sockaddr_storage sock_info_{};
  std::string name_;
  Channel channel_;


 public:
  using Ptr = std::shared_ptr<Connection>;
  using Buffer = std::vector<char>;
  constexpr static auto BUFFER_SIZE = 4 * 1024;
  Connection(Connection&& conn)
      : loop_(conn.loop_),
        fd_(std::exchange(conn.fd_, -1)),
        sock_info_(conn.sock_info_),
        name_(std::move(conn.name_)),
        channel_(conn.channel_) {}
  Connection(EventLoop* loop, int fd, std::string_view name)
      : loop_(loop), fd_(fd), name_(name), channel_(fd) {
    socklen_t addrlen = sizeof(sock_info_);
    getsockname(fd_, reinterpret_cast<sockaddr*>(&sock_info_), &addrlen);
  }

  Connection(EventLoop* loop, int fd, const sockaddr_storage& info,
             std::string_view name)
      : loop_(loop), fd_(fd), sock_info_(info), name_(name), channel_(fd) {}

  Task<Buffer> read(ssize_t size = -1);
  Task<Buffer> readn(ssize_t size);
  Task<bool> write(const Buffer& buf);
  Task<Buffer> read_until_eof();

  std::string get_address() {
    auto addr = reinterpret_cast<sockaddr_in*>(&sock_info_);
    return fmt::format("{}.{}.{}.{}:{}", addr->sin_addr.s_addr & 0xff,
                       addr->sin_addr.s_addr >> 8 & 0xff,
                       addr->sin_addr.s_addr >> 16 & 0xff,
                       addr->sin_addr.s_addr >> 24 & 0xff, addr->sin_port);
  }
  Channel* channel() { return &channel_; }

  void set_tcp_no_delay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval,
                 static_cast<socklen_t>(sizeof optval));
  }

  auto name() { return name_; }

  void shutdown_write() { ::shutdown(fd_, SHUT_WR); }

  ~Connection() {
    if (fd_ > 0) {
      ::close(fd_);
    }
  }
};

}  // namespace mynet