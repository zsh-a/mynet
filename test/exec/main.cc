
#include <iostream>
#include <string>

#include "fmt/format.h"
// #include "mynet/generator.h"
#include <memory>

#include "mynet/event_loop.h"
#include "mynet/sleep.h"
#include "mynet/task.h"
#include "mynet/sockets.h"
using namespace std;
using namespace mynet;
// using IntGenerator = Generator<int>;

// IntGenerator gen() {
//   for (int i = 0; i < 100; i++) co_yield i;
// }

Task<std::string> hello() {
  fmt::print("enter hello\n");

  co_await mynet::sleep(2000);
  fmt::print("leave hello\n");
  co_return "hello";
}

Task<std::string> world() {
  fmt::print("enter world\n");
  co_await mynet::sleep(1000);
  fmt::print("leave world\n");
  co_return "world";
}

Task<std::string> helloworld() {
  auto h = mynet::create_task(hello());
  auto w = mynet::create_task(world());
  co_return fmt::format("{} {}", h.get_result(), w.get_result());
}

// Task<std::string> run_once() {
//   auto a = co_await hello();
//   auto b = co_await world();
//   fmt::print("{} {} \n", a, b);
// }

NoWaitTask<> run_sleep() {
  co_await mynet::sleep(2000);
  fmt::print("timeout\n");
}

Task<bool> conn_test(){
  auto conn = co_await open_connection("127.0.0.1",8000);
  // co_await mynet::sleep(5000);
  fmt::print("xxx {} \n",conn.fd_);
  // co_await mynet::sleep(10000);
  auto buf = co_await conn.read(100);

  for(auto x : buf) fmt::print("{}",x);
  fmt::print("\n");
  co_return true;
}

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

  {
    auto& loop = EventLoop::get();
    mynet::create_task(conn_test());
    loop.run();

    // Epoller poller{};
    // fmt::print("fd {} \n",poller.fd_);
    // poller.register_event(Event{.fd = 0,.events = EPOLLIN});
  }
}
