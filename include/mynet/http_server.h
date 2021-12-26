#pragma once
#include "mynet/http_context.h"
#include "mynet/tcp_server.h"
namespace mynet {

namespace http {

template <typename HttpHandler>
class HttpServer {
  EventLoop* loop_;
  std::string ip_;
  int port_;
  std::string name_;
  HttpHandler http_handler_;
  int num_threads_;

 public:
  HttpServer(EventLoop* loop, const std::string_view& ip, int port,
             const std::string_view& name, HttpHandler http_handler,
             int num_threads = 0)
      : loop_(loop),
        ip_(ip),
        port_(port),
        name_(name),
        http_handler_(std::move(http_handler)),
        num_threads_(num_threads) {}
  Task<bool> serve() {
    auto handler = [this](Connection::Ptr conn) -> Task<bool> {
      HttpContext ctx;
      co_await ctx.process_http(conn, http_handler_).run_in(conn->loop_);
      co_return true;
    };
    auto server = co_await start_tcp_server(loop_, ip_, port_, handler, name_,
                                            num_threads_)
                      .run_in(loop_);
    co_await server.serve().run_in(loop_);
    co_return true;
  }

  //   static Task<bool> http(Connection::Ptr conn) {
  //     HttpContext ctx;
  //     co_await ctx.process_http(conn, handler_).run_in(conn->loop_);
  //     conn->shutdown_write();
  //     co_return true;
  //   }
};

}  // namespace http

}  // namespace mynet