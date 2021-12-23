
#include <iostream>
#include <memory>
#include <string>

#include "fmt/format.h"
#include "fmt/os.h"
#include "mynet/event_loop.h"
#include "mynet/http/http_request.h"
#include "mynet/http/http_response.h"
#include "mynet/sleep.h"
#include "mynet/sockets.h"
#include "mynet/task.h"
#include "mynet/tcp_server.h"
#include "mynet/event_loop_thread_pool.h"
using namespace std;
using namespace mynet;

EventLoop g_loop;
EventLoopThreadPool pool{&g_loop,2};


std::unordered_map<std::string, HttpRequest::Method> methods{
    {"GET", HttpRequest::Method::GET}, {"POST", HttpRequest::Method::POST}};

std::unordered_map<std::string, Version> versions{
    {"HTTP/1.1", Version::HTTP11}};
HttpRequest parse(Connection::Buffer buf) {
  // fmt::print("header : {}\n",string(buf.begin(),buf.end()));
  auto n = buf.size();
  HttpRequest req{};
  size_t i = 0, last = 0;
  while (i < n && isalpha(buf[i])) {
    ++i;
  }
  req.method_ = methods[string(buf.begin() + last, buf.begin() + i)];
  while (i < n && isspace(buf[i])) ++i;
  last = i;
  while (i < n && !isspace(buf[i])) ++i;
  req.url_ = string(buf.begin() + last, buf.begin() + i);
  while (i < n && isspace(buf[i])) ++i;
  last = i;
  while (i < n && buf[i] != '\r') ++i;
  req.version_ = versions[string(buf.begin() + last, buf.begin() + i)];
  i += 2;
  while (i < n && buf[i] != '\r') {
    last = i;
    while (i < n && buf[i] != ':') ++i;
    auto name = string(buf.begin() + last, buf.begin() + i);
    ++i;
    while (i < n && isalpha(buf[i])) ++i;
    last = i;
    while (i < n && buf[i] != '\r') ++i;
    auto value = string(buf.begin() + last, buf.begin() + i);
    i += 2;
    req.headers_[name] = value;
  }
  return req;
}

Connection::Buffer to_buffer(HttpResponse resp) {
  Connection::Buffer buf(1024);
  string ver{"HTTP/1.1"};
  size_t i = 0;
  std::copy(ver.begin(), ver.end(), buf.begin() + i);
  i += ver.size();
  buf[i++] = ' ';
  buf[i++] = '2';
  buf[i++] = '0';
  buf[i++] = '0';
  buf[i++] = ' ';
  buf[i++] = 'O';
  buf[i++] = 'K';
  buf[i++] = '\r';
  buf[i++] = '\n';
  for (auto&& [k, v] : resp.headers_) {
    std::copy(k.begin(), k.end(), buf.begin() + i);
    i += k.size();
    buf[i++] = ':';
    buf[i++] = ' ';
    std::copy(v.begin(), v.end(), buf.begin() + i);
    i += v.size();
    buf[i++] = '\r';
    buf[i++] = '\n';
  }
  buf[i++] = '\r';
  buf[i++] = '\n';
  std::copy(resp.body_.begin(), resp.body_.end(), buf.begin() + i);
  i += resp.body_.size();
  buf.resize(i);
  // fmt::print("{} \n",string(buf.begin(),buf.end()));
  return buf;
}

Task<bool> http(Connection::Ptr conn) {
  auto buf = co_await conn->read(1024).run_in(conn->loop_);
  auto req = parse(std::move(buf));
  // fmt::print("{} \n", req.url_);
  // for (auto&& [k, v] : req.headers_) {
  //   fmt::print("{} : {} \n", k, v);
  // }

  HttpResponse resp;

  resp.version_ = req.version_;
  resp.status_code_ = HttpResponse::StatusCode::k200Ok;
  string msg = "<h1>hello</h1><hr/><h2>world</h2>";
  resp.body_ = msg;
  resp.headers_["Content-Type"] = "text/html";
  resp.headers_["Content-Length"] = to_string(msg.size());

  buf = to_buffer(std::move(resp));
  co_await conn->write(buf).run_in(conn->loop_);
  conn->shutdown();
  // fmt::print("~~~~\n");
  co_return true;
}

Task<bool> http_server_test() {
  auto server =
      co_await start_tcp_server(&g_loop,"0.0.0.0", 8080, http, "tinyhttp_server").run_in(&g_loop);
  co_await server.serve().run_in(&g_loop);
}

int main() {
  {

    g_loop.create_task(http_server_test());
    g_loop.run();
  }
}
