
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

Task<bool> client() {
  auto conn = co_await mynet::open_connection(&g_loop, "0.0.0.0", 9999);
  Connection::Buffer buf(16);
  while (1) {
    auto now = duration_cast<chrono::microseconds>(
                   chrono::system_clock::now().time_since_epoch())
                   .count();
    memcpy(buf.data(), &now, sizeof now);
    co_await conn.write(buf);
    auto recv_buf = co_await conn.readn(16);
    int64_t message[2]{};
    memcpy(message, recv_buf.data(), 16);
    int64_t send = message[0];
    int64_t their = message[1];
    int64_t back = conn.channel()->event_time().count();
    int64_t mine = (back + send) / 2;
    fmt::print("round trip : {}  clock error : {}\n", back - send,
               their - mine);
    co_await mynet::sleep(&g_loop, milliseconds(500));
  }
  conn.shutdown_write();
  co_return true;
}

int main(int argc, char* argv[]) {
  {
    g_loop.create_task(client());
    g_loop.run();
  }
}
