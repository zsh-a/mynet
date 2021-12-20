
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

Task<bool> rtt_server(Connection::Ptr conn){
  while(1){
    auto buf = co_await conn->readn(16);
    if(buf.size() == 0) {
      break;
    }
    auto recv_time = conn->channel()->event_time().count();
    memcpy(buf.data() + 8,&recv_time,sizeof(recv_time));
    co_await conn->write(buf);
  }
  co_return true;
}


Task<bool> rtt_server_test(){

  auto server = co_await start_tcp_server("0.0.0.0",9999,rtt_server,"rtt_server");
  co_await server.serve();
}

int main() {
  {
    auto& loop = EventLoop::get();
    mynet::create_task(rtt_server_test());
    loop.run();
  }
}
