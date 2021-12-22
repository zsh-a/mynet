
#include <iostream>
#include <memory>
#include <string>

#include "fmt/format.h"
#include "fmt/os.h"
#include "mynet/event_loop.h"
#include "mynet/sleep.h"
#include "mynet/sockets.h"
#include "mynet/task.h"
#include "mynet/tcp_server.h"
using namespace std;
using namespace mynet;

EventLoop g_loop;

Task<bool> tunnel(Connection::Ptr client, Connection::Ptr server) {
  while (1) {
    auto buf = co_await client->read(16 * 1024);
    if (buf.size() == 0) {
      break;
    }
    co_await server->write(buf);
  }
  server->shutdown_write();
}

Task<bool> relay(Connection::Ptr conn) {
  auto server = co_await open_connection(&g_loop,"0.0.0.0", 3000);
  auto ptr = make_shared<Connection>(std::move(server));
  g_loop.create_task(tunnel(conn, ptr));
  g_loop.create_task(tunnel(ptr, conn));
  co_return true;
}

Task<bool> relay_server_test() {
  auto server = co_await start_tcp_server(&g_loop, "0.0.0.0", 9999, relay,
                                          "relay_server");
  co_await server.serve();
}

int main() {
  {
    g_loop.create_task(relay_server_test());
    g_loop.run();
  }
}
