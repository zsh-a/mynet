
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

Task<HttpResponse> hello(HttpRequest req) {
  HttpResponse resp{};
  if(req.url_ == "/hello")
    resp.set_body("<p>Hello, world!</p>");
  else{
    resp = HttpResponse::get_error_resp(404,"Now found 🤔");
  }
  co_return resp;
}

int main() {
  {
    HttpServer http_server{&g_loop, "0.0.0.0", 9999, "http server",hello,2};
    g_loop.create_task(http_server.serve());
    g_loop.run();
  }
}
