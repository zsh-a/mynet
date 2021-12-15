
#include <iostream>
#include <string>

#include "fmt/format.h"
// #include "mynet/generator.h"
#include <memory>

#include "mynet/event_loop.h"
#include "mynet/sleep.h"
#include "mynet/task.h"
using namespace std;
using namespace mynet;
// using IntGenerator = Generator<int>;

// IntGenerator gen() {
//   for (int i = 0; i < 100; i++) co_yield i;
// }

Task<std::string> hello() { co_return "hello"; }

Task<std::string> world() { co_return "world"; }

Task<std::string> helloworld() {
  auto a = co_await hello();
  auto b = co_await world();
  co_return fmt::format("{} {}", a, b);
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

std::vector<int> v;

Task<int> vec1() { v.push_back(1); }
Task<int> vec2() { v.push_back(2); }

Task<int> run_vec() {
  v.push_back(3);
  co_await vec1();
  v.push_back(4);
  co_await vec2();
}

struct Dummy {};
Task<Dummy> coro1(std::vector<int>& result) {
  result.push_back(1);
  co_return Dummy{};
}

Task<Dummy> coro2(std::vector<int>& result) {
  result.push_back(2);
  co_await coro1(result);
  result.push_back(20);
  co_return Dummy{};
}

Task<Dummy> coro3(std::vector<int>& result) {
  result.push_back(3);
  co_await coro2(result);
  result.push_back(30);
  co_return Dummy{};
}

Task<Dummy> coro4(std::vector<int>& result) {
  result.push_back(4);
  co_await coro3(result);
  result.push_back(40);
  co_return Dummy{};
}

int main() {
  {
    auto task = hello();
    while (!task.done()) task.resume();
    fmt::print("{}\n", task.get_result());
  }

  {
    auto task = helloworld();
    while (!task.done()) {
      fmt::print("resume\n");
      task.resume();
    }
    fmt::print("{}\n", task.get_result());
  }

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

  {
    std::vector<int> result;
    auto g = coro2(result);
    while (!g.done()) {
      for (auto x : result) {
        fmt::print("{} ", x);
      }
      fmt::print("\n");
      fmt::print("resume\n");
      g.resume();
    }
  }
  // {
  //   auto& loop = EventLoop::get();
  //   auto g = run_once();
  //   // loop.run_until_done(&g);
  //   loop.run_delay(std::chrono::milliseconds{2000}.count(),g.get_resumable());
  //   loop.run();
  //   fmt::print("{} \n", loop.time());
  // }

  {
    auto g = run_sleep();
    auto& loop = EventLoop::get();
    loop.run();
  }
}