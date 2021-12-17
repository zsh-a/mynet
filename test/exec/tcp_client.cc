
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

Task<bool> echo_client() {
  auto& loop = EventLoop::get();
  auto conn = co_await open_connection("127.0.0.1", 9999);
  std::string s;
  while (1) {
    getline(cin,s);
    co_await conn.write(mynet::Connection::Buffer(s.begin(),s.end()));
    auto buf = co_await conn.read(128);
    cout << string(buf.begin(),buf.end()) <<"\n";
  }
  co_return true;
}
int main() {
  { auto& loop = EventLoop::get();
    mynet::create_task(echo_client());
    loop.run();
   }
}
