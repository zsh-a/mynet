
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
EventLoop g_loop;
int cnt = 100000;
Task<bool> client(){
  auto conn = co_await mynet::open_connection(&g_loop,"127.0.0.1",9999).run_in(&g_loop);
  co_await conn.write(buf).run_in(&g_loop);
  size_t tot = 0;
  for(int i = 0;i < cnt;i++){
    auto buf = co_await conn.read(block_size).run_in(&g_loop);
    tot += buf.size();
    if(buf.size() == 0) break;
    // fmt::print("receiving data {}\n",buf.data());
    if(!co_await conn.write(buf).run_in(&g_loop)) break; 
  }
  conn.shutdown_write();
  // do{
    // auto buf = co_await conn.read(block_size);
    // tot += buf.size();
  // }while(buf.size() != 0);
  fmt::print("read {} bytes\n",tot);
  co_return true;
}

int main(int argc,char* argv[]) {

  for(int i = 0;i < block_size;i++) buf[i] = i % 128;
  {
    for(int i = 0;i < atoi(argv[1]);i++)
          g_loop.create_task(client());
    g_loop.run();
  }
}
