
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


Task<bool> chat(Connection::Ptr conn,std::map<std::string,Connection::Ptr>& peers){
  while(1){
    auto buf = co_await conn->read(4);
    auto head = buf;
    if(buf.size() == 0) break;
    auto n32 =  *reinterpret_cast<uint32_t*>(buf.data());
    auto msg_len = ntohl(n32);
    if(msg_len > 0xffff){
      log::Log(log::Error,"invalid length {}",msg_len);
      conn->shutdown_write();
      break;
    }

    buf = co_await conn->read(msg_len);
    head.insert(head.end(),buf.begin(),buf.end());
    for(auto&& [name,conn]:peers){
      co_await conn->write(head);
    }
    log::Log(log::Info,"from {} : message {}. length : {}",conn->name(),string(buf.begin(),buf.end()),msg_len);
  }
  log::Log(log::Info,"user : {} leave",conn->name());
  peers.erase(conn->name());
  co_return true;
}

Task<bool> chat_server(){
  std::map<std::string,Connection::Ptr> peers;

  auto server = co_await start_tcp_server("0.0.0.0",9999,chat,"chat server");
  for(;;){
    auto conn = co_await server.accept();
    peers[conn->name()] = conn;
    mynet::create_task(chat(conn,peers));
  }
}

int main() {
  {
    auto& loop = EventLoop::get();
    mynet::create_task(chat_server());
    loop.run();
  }
}
