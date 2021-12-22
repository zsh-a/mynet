
#include <iostream>
#include <string>

#include "fmt/format.h"
// #include "mynet/generator.h"
#include <memory>

#include "codec.h"
#include "fmt/os.h"
#include "mynet/event_loop.h"
#include "mynet/sleep.h"
#include "mynet/sockets.h"
#include "mynet/task.h"
#include "mynet/tcp_server.h"
using namespace std;
using namespace mynet;

EventLoop g_loop;
Task<bool> send(Connection::Ptr conn) {
  Channel channel(0);
  while (1) {
    co_await channel.read(&g_loop);
    char msg[128]{};
    int msg_len = ::read(0, msg, sizeof msg);
    --msg_len;
    auto buf = Connection::Buffer(4 + msg_len);
    auto n32 = htonl(msg_len);
    memcpy(buf.data(), &n32, 4);
    for (int i = 0; i < msg_len; i++) buf[i + 4] = msg[i];
    co_await conn->write(buf)(conn->loop_);
  }
}

Task<bool> recv(Connection::Ptr conn) {
  while (1) {
    auto [state, msg] = co_await decode(conn)(conn->loop_);
    if (state == State::DISCONNECTED) {
      conn->shutdown_write();
      break;
    }
    fmt::print("received boardcast : {}\n", msg);
  }
}

Task<bool> client() {
  auto conn = co_await mynet::open_connection(&g_loop,"127.0.0.1", 9999)(&g_loop);;
  auto ptr = make_shared<Connection>(std::move(conn));
  g_loop.create_task(send(ptr));
  g_loop.create_task(recv(ptr));
  co_return true;
}

int main(int argc, char* argv[]) {
  {
    g_loop.create_task(client());
    g_loop.run();
  }
}
