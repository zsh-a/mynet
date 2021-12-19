
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

Task<bool> client(){
  auto conn = co_await mynet::open_connection("192.3.127.120",9999);
  Connection::Buffer buf(16);
  while(1){
    auto now = duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    memcpy(buf.data(),&now,sizeof now);
    co_await conn.write(buf);
    auto recv_buf = co_await conn.read(16);
    int64_t message[2]{};
    memcpy(message,recv_buf.data(),16);
    int64_t send = message[0];
    int64_t their = message[1];
    int64_t back = conn.channel()->event_time().count();
    int64_t mine = (back+send)/2;
    fmt::print("round trip : {}  clock error : {}\n",back - send,their - mine);
    co_await mynet::sleep(milliseconds(500));
  }
  conn.shutdown_write();
  co_return true;
}

int main(int argc,char* argv[]) {

  {
    auto& loop = EventLoop::get();
    mynet::create_task(client());
    loop.run();
  }
}