
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
// using IntGenerator = Generator<int>;

// IntGenerator gen() {
//   for (int i = 0; i < 100; i++) co_yield i;
// }
EventLoop g_loop;
Task<std::string> hello() {
  fmt::print("enter hello\n");

  co_await mynet::sleep(&g_loop,chrono::milliseconds(1000));
  fmt::print("leave hello\n");
  co_return "hello";
}

Task<std::string> world() {
  fmt::print("enter world\n");
  co_await mynet::sleep(&g_loop,chrono::milliseconds(1000));
  fmt::print("leave world\n");
  co_return "world";
}

Task<std::string> helloworld() {
  auto h = g_loop.create_task(hello());
  auto w = g_loop.create_task(world());
  co_return fmt::format("{} {}", h.get_result(), w.get_result());
}

// Task<std::string> run_once() {
//   auto a = co_await hello();
//   auto b = co_await world();
//   fmt::print("{} {} \n", a, b);
// }

NoWaitTask<> run_sleep() {
  co_await mynet::sleep(&g_loop,chrono::milliseconds(2000));
  fmt::print("timeout\n");
}

Task<bool> conn_test(){
  auto conn = co_await open_connection(&g_loop,"127.0.0.1",8000);
  // co_await mynet::sleep(5000);
  // co_await mynet::sleep(10000);
  auto buf = co_await conn.read_until_eof();
  // for(auto x : buf) fmt::print("{}",x);
  // fmt::print("\n");
  fmt::print("read {} bytes\n",buf.size());
  auto out = fmt::output_file("guide.txt");
  out.print("{}", string(buf.begin(),buf.end()));
  co_return true;
}

Task<bool> echo_server(Connection::Ptr conn){
  while(1){
    auto buf = co_await conn->read(4* 1024);
    if(buf.size() == 0) break;
    // fmt::print("receiving data {}\n",buf.data());
    if(!co_await conn->write(buf)) break; 
  }
  co_return true;
}


Task<bool> tcp_server_test(){
  Connection::Buffer buf{'h','e','l','l','o','\n'};
  auto server = co_await start_tcp_server(&g_loop,"0.0.0.0",9999,echo_server,"tcp server");
  co_await server.serve();
}

// Task<int> factorial(int n) {
//     if (n <= 1) {
//         co_await dump_callstack();
//         co_return 1;
//     }
//     co_return (co_await factorial(n - 1)) * n;
// }

// int main() {
//   auto& loop = EventLoop::get();
//   mynet::create_task(factorial(10));
//     // fmt::print("run result: {}\n", mynet::create_task(factorial(10)));
//     loop.run();
//     return 0;
// }


int main() {
  // {
  //   auto task = hello();
  //   while (!task.done()) task.resume();
  //   fmt::print("{}\n", task.get_result());
  // }

  // {
  //   auto& loop = EventLoop::get();
  //   auto task = helloworld();
  //   loop.run_immediately(task.get_resumable());
  //   fmt::print("{}\n", task.get_result());
  // }

  // {
  //   auto g = run_vec();
  //   while (!g.done()) {
  //     for (auto x : v) {
  //       fmt::print("{} ", x);
  //     }
  //     fmt::print("\n");
  //     fmt::print("resume\n");
  //     g.resume();
  //   }
  // }

  // {
  //   std::vector<int> result;
  //   auto& loop = EventLoop::get();
  //   loop.run_until_done(coro2(result).get_resumable());
  //   for (auto x : result) {
  //     fmt::print("{} ", x);
  //   }
  // }
  // {
  //   auto& loop = EventLoop::get();
  //   auto g = run_once();
  //   // loop.run_until_done(&g);
  //   loop.run_delay(std::chrono::milliseconds{2000}.count(),g.get_resumable());
  //   loop.run();
  //   fmt::print("{} \n", loop.time());
  // }

  // {
  //   auto& loop = EventLoop::get();
  //   mynet::create_task(conn_test());
  //   loop.run();
  // }

  {
    g_loop.create_task(tcp_server_test());
    g_loop.run();
  }

}
