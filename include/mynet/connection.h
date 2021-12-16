#pragma once

#include "mynet/task.h"

namespace mynet {

class Connection : NonCopyable {
public:
  int fd_{-1};
  sockaddr_storage sock_info_{};

 public:
  using Buffer = std::vector<char>;
  constexpr static auto BUFFER_SIZE = 4 * 1024;
  Connection(Connection&& conn)
      : fd_(std::exchange(conn.fd_, -1)), sock_info_(conn.sock_info_) {}
  Connection(int fd) : fd_(fd) {
    socklen_t addrlen = sizeof(sock_info_);
    getsockname(fd_, reinterpret_cast<sockaddr*>(&sock_info_), &addrlen);
  }

  Connection(int fd,const sockaddr_storage& info) : fd_(fd),sock_info_(info) {
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
    res.resize(tot_read);
    // fmt::print("tot_read read {} bytes\n",tot_read);
    co_return res;
  }
  Task<bool> write(const Buffer& buf) {
    Event ev{.fd = fd_, .events = EPOLLOUT};
    ssize_t tot_writen = 0;
    auto& loop = EventLoop::get();
    while(tot_writen < buf.size()){
      co_await loop.wait_event(ev);
      ssize_t writen = ::write(fd_,buf.data() + tot_writen,buf.size() - tot_writen);
      if(writen < 0){
        perror("write data failed");
        co_return false;
        // exit(errno);
      }
      tot_writen += writen;
    }
    co_return true;
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
      // fmt::print("read {} bytes return {} \n",tot_read,cur_read);
      if (cur_read == 0) break;
      buf.resize(cur_read);
      res.insert(res.end(), buf.begin(), buf.end());
      tot_read += cur_read;
    }
    
    co_return res;
  }

  ~Connection() {
    if (fd_ > 0) {
      ::close(fd_);
    }
  }
};

}  // namespace mynet