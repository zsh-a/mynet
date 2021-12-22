
#include <iostream>
#include <string>

#include "fmt/format.h"
// #include "mynet/generator.h"
#include <memory>

#include "fmt/os.h"
#include "mynet/event_loop.h"
#include "mynet/sleep.h"
#include "mynet/sockets.h"
#include "mynet/task.h"
#include "mynet/tcp_server.h"
using namespace std;
using namespace mynet;

EventLoop g_loop;

Task<bool> rtt_server(Connection::Ptr conn) {
  while (1) {
    auto buf = co_await conn->readn(16);
    if (buf.size() == 0) {
      break;
    }
    auto recv_time = conn->channel()->event_time().count();
    memcpy(buf.data() + 8, &recv_time, sizeof(recv_time));
    co_await conn->write(buf);
  }
  co_return true;
}

Task<bool> rtt_server_test() {
  auto server = co_await start_tcp_server(&g_loop, "0.0.0.0", 9999, rtt_server,
                                          "rtt_server");
  co_await server.serve();
}

int main() {
  {
    g_loop.create_task(rtt_server_test());
    g_loop.run();
  }
}
