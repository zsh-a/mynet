
#include <iostream>
#include <memory>
#include <string>

#include "fmt/format.h"
#include "fmt/os.h"
#include "mynet/event_loop.h"
#include "mynet/event_loop_thread_pool.h"
#include "mynet/http_context.h"
#include "mynet/http_server.h"
#include "mynet/sleep.h"
#include "mynet/sockets.h"
#include "mynet/task.h"
using namespace std;
using namespace mynet;
using namespace mynet::http;
EventLoop g_loop;
EventLoopThreadPool pool{&g_loop, 2};

Task<HttpResponse> hello(HttpRequest req) {
  HttpResponse resp{};
  resp.set_body("<p>Hello, world!</p>");
  co_return resp;
}
// TODO better api
Task<bool> handler(Connection::Ptr conn) {
  HttpContext ctx;
  co_await ctx.process_http(conn, hello).run_in(conn->loop_);
  conn->shutdown_write();
  co_return true;
}

int main() {
  {
    HttpServer http_server{&g_loop, "0.0.0.0", 8080, "http server", handler};
    g_loop.create_task(http_server.serve());
    g_loop.run();
  }
}
