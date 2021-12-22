
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

#include "codec.h"
#include "fmt/os.h"
using namespace std;
using namespace mynet;

int block_size = 16 * 1024;
Connection::Buffer buf(block_size);

EventLoop g_loop;

Task<bool> chat(Connection::Ptr conn,std::map<std::string,Connection::Ptr>& peers){
  while(1){
    auto [state,msg] =  co_await decode(conn);
    if(state == State::DISCONNECTED) {
      conn->shutdown_write();
      break;
    }
    auto buf = encode(msg);
    for(auto&& [name,conn]:peers){
      co_await conn->write(buf);
    }
    log::Log(log::Info,"from {} : message {}. length : {}",conn->name(),msg,msg.size());
  }
  log::Log(log::Info,"user : {} leave",conn->name());
  peers.erase(conn->name());
  co_return true;
}

Task<bool> chat_server(){
  std::map<std::string,Connection::Ptr> peers;

  auto server = co_await start_tcp_server(&g_loop,"0.0.0.0",9999,chat,"chat server");
  for(;;){
    auto conn = co_await server.accept(&g_loop);
    peers[conn->name()] = conn;
    g_loop.create_task(chat(conn,peers));
  }
}

int main() {
  {
    g_loop.create_task(chat_server());
    g_loop.run();
  }
}
