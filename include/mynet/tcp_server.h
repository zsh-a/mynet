#pragma once
#include <list>
#include <map>
#include <memory>

#include "fmt/color.h"
#include "mynet/sockets.h"
#include "mynet/task.h"
namespace mynet {

template <typename Callback>
class TcpServer {
  int fd_{-1};
  Channel channel_;
  std::string name_;
  Callback cb_;

  uint32_t conn_id{0};
  std::map<std::string, Connection::Ptr> connections_map_;

 public:
  static constexpr int DEFAULT_MAX_CONNECTIONS = 8;
  TcpServer(int fd, std::string_view name, Callback cb)
      : fd_(fd), channel_(fd), name_(name), cb_(std::forward<Callback>(cb)) {}
  TcpServer(TcpServer&& other)
      : fd_(std::exchange(other.fd_, -1)),
        channel_(other.channel_),
        name_(std::move(other.name_)),
        cb_(other.cb_) {}
  Task<Connection> serve() {
    auto& loop = EventLoop::get();
    // std::list<Task<bool>> connections;

    std::vector<Task<bool>> connected;

    auto gc = [&]() {
      if (connected.size() < 100) [[likely]] {
        return;
      }
      for (auto iter = connected.begin(); iter != connected.end();) {
        if (iter->done()) {
          iter = connected.erase(iter);
        } else {
          ++iter;
        }
      }
    };
    while (1) {
      co_await channel_.read(&loop);
      sockaddr_storage remote_addr{};
      socklen_t sock_len = sizeof(remote_addr);
      int client_fd =
          ::accept(fd_, reinterpret_cast<sockaddr*>(&remote_addr), &sock_len);
      if (client_fd == -1) continue;
      auto addr = reinterpret_cast<sockaddr_in*>(&remote_addr);
      auto ip_port = fmt::format(
          "{}.{}.{}.{}:{}", addr->sin_addr.s_addr & 0xff,
          addr->sin_addr.s_addr >> 8 & 0xff, addr->sin_addr.s_addr >> 16 & 0xff,
          addr->sin_addr.s_addr >> 24 & 0xff, addr->sin_port);
      std::string conn_name =
          fmt::format("{}-{}#{}", name_, ip_port, conn_id++);
      log::Log(log::Info, "received connection from {}", conn_name);

      auto conn =
          std::make_shared<Connection>(client_fd, remote_addr, conn_name);
      connected.emplace_back(mynet::create_task(cb_(std::move(conn))));
      gc();
      // connections_map_[conn_name] = conn;
    }
  }

  Task<Connection::Ptr> accept() {
    auto& loop = EventLoop::get();
    co_await channel_.read(&loop);
    sockaddr_storage remote_addr{};
    socklen_t sock_len = sizeof(remote_addr);
    int client_fd =
        ::accept(fd_, reinterpret_cast<sockaddr*>(&remote_addr), &sock_len);
    if (client_fd < 0) {
      perror("accept");
      log::Log(log::Error, "accept error");
      co_return nullptr;
    }
    auto addr = reinterpret_cast<sockaddr_in*>(&remote_addr);
    auto ip_port = fmt::format(
        "{}.{}.{}.{}:{}", addr->sin_addr.s_addr & 0xff,
        addr->sin_addr.s_addr >> 8 & 0xff, addr->sin_addr.s_addr >> 16 & 0xff,
        addr->sin_addr.s_addr >> 24 & 0xff, addr->sin_port);
    std::string conn_name = fmt::format("{}-{}#{}", name_, ip_port, conn_id++);
    log::Log(log::Info, "received connection from {}", conn_name);

    auto conn = std::make_shared<Connection>(client_fd, remote_addr, conn_name);
    // connections_map_[conn_name] = conn;
    co_return conn;
  }

  ~TcpServer() {
    if (fd_ > 0) ::close(fd_);
  }
};

template <typename Callback>
Task<TcpServer<Callback>> start_tcp_server(std::string_view ip, int port,
                                           Callback cb, std::string_view name) {
  addrinfo hints{.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
  addrinfo* server_info{nullptr};
  auto port_str = std::to_string(port);
  // TODO: getaddrinfo is a blocking api
  if (int rv = getaddrinfo(ip.data(), port_str.c_str(), &hints, &server_info);
      rv != 0) {
    perror("getaddrinfo");
    exit(errno);
  }
  internel::AddressInfo addr(server_info);

  int server_fd = -1;
  for (auto p = server_info; p != nullptr; p = p->ai_next) {
    if ((server_fd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK,
                            p->ai_protocol)) == -1) {
      continue;
    }
    int yes = 1;
    // lose the pesky "address already in use" error message
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (bind(server_fd, p->ai_addr, p->ai_addrlen) == 0) {
      break;
    }
    close(server_fd);
    server_fd = -1;
  }
  if (server_fd == -1) {
    fmt::print("start server failed {}\n", errno);
    exit(errno);
  }

  if (::listen(server_fd, TcpServer<Callback>::DEFAULT_MAX_CONNECTIONS) == -1) {
    fmt::print("server listen failed {}\n", errno);
    exit(errno);
  }
  co_return TcpServer{server_fd, name, cb};
}

}  // namespace mynet