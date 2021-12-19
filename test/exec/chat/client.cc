
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

Task<bool> send(Connection::Ptr conn){

  Channel channel(0);
  auto& loop = EventLoop::get(); 
  while (1) {
    co_await channel.read(&loop);
    char msg[128]{};
    int msg_len = ::read(0,msg,sizeof msg);
    --msg_len;
    auto buf = Connection::Buffer(4 + msg_len);
    auto n32 = htonl(msg_len);
    memcpy(buf.data(), &n32, 4);
    for (int i = 0; i < msg_len; i++) buf[i + 4] = msg[i];
    co_await conn->write(buf);
  }
}

Task<bool> recv(Connection::Ptr conn){
  while (1) {
    auto buf = co_await conn->read(4);
    // fmt::print("receive msg len : {}\n",buf.size());
    if(buf.size() == 0) break;
    auto n32 =  *reinterpret_cast<uint32_t*>(buf.data());
    auto msg_len = ntohl(n32);
    if(msg_len > 0xffff){
      log::Log(log::Error,"invalid length {}",msg_len);
      conn->shutdown_write();
      break;
    }
    buf = co_await conn->read(msg_len);
    fmt::print("received boardcast : {}\n",string(buf.begin(),buf.end()));
  }
}

Task<bool> client() {
  auto conn = co_await mynet::open_connection("127.0.0.1", 9999);
  auto ptr = make_shared<Connection>(std::move(conn));
  mynet::create_task(send(ptr));
  mynet::create_task(recv(ptr));
  co_return true;
}

int main(int argc, char* argv[]) {
  {

    auto& loop = EventLoop::get();
    mynet::create_task(client());
    loop.run();
  }
}
