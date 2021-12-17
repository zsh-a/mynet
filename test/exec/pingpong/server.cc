
#include <iostream>
#include <string>

#include "fmt/format.h"
// #include "mynet/generator.h"
#include <memory>

#include "mynet/event_loop.h"
#include "mynet/sleep.h"
#include "mynet/task.h"
#include "mynet/sockets.h"
#include "mynet/tcp_server.h"
#include "fmt/os.h"
using namespace std;
using namespace mynet;

int block_size = 16 * 1024;
Connection::Buffer buf(block_size);


Task<bool> echo_server(Connection conn){
  size_t tot = 0;
  while(1){
    auto buf = co_await conn.read(block_size);
    if(buf.size() == 0) {
      break;
    }
    tot += buf.size();
    // fmt::print("receiving data {} bytes\n",buf.size());
    if(!co_await conn.write(buf)) {
      break;
    }
  }
  fmt::print("recevide {} bytes\n",tot);
  // log::Log(log::Info,"received {} bytes",tot);
  co_return true;
}


Task<bool> tcp_server_test(){

  auto server = co_await start_tcp_server("0.0.0.0",9999,echo_server);
  co_await server.serve();
}

int main() {
  {
    auto& loop = EventLoop::get();
    mynet::create_task(tcp_server_test());
    loop.run();
  }
}