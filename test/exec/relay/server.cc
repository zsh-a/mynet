
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
    auto buf = co_await client->read(16 * 1024).run_in(client->loop_);
    if (buf.size() == 0) {
      break;
    }
    co_await server->write(buf).run_in(server->loop_);
  }
  client->close();
  server->close();
}

Task<bool> relay(Connection::Ptr conn) {
  auto server = co_await open_connection(conn->loop_,"0.0.0.0", 3000).run_in(conn->loop_);
  auto ptr = make_shared<Connection>(std::move(server));
  conn->loop_->queue_in_loop(tunnel(conn, ptr));
  conn->loop_->queue_in_loop(tunnel(ptr, conn));
  co_return true;
}

Task<bool> relay_server_test() {
  auto server = co_await start_tcp_server(&g_loop, "0.0.0.0", 9999, relay,
                                          "relay_server").run_in(&g_loop);
  co_await server.serve().run_in(&g_loop);
}

int main() {
  {
    g_loop.create_task(relay_server_test());
    g_loop.run();
  }
}
