
#include <iostream>
#include <string>

#include "fmt/format.h"
// #include "mynet/generator.h"
#include <memory>

#include "mynet/event_loop.h"
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

Task<std::string> run_once() {
  auto a = co_await hello();
  auto b = co_await world();
  fmt::print("{} {} \n", a, b);
}

int main() {
  {
    auto task = hello();
    while (!task.done()) task.resume();
    fmt::print("{}\n", task.get_result());
  }

  {
    auto task = helloworld();
    while (!task.done()) task.resume();
    fmt::print("{}\n", task.get_result());
  }

  {
    auto& loop = EventLoop::get();
    auto g = run_once();
    // loop.run_until_done(&g);
    loop.run_delay(std::chrono::milliseconds{2000}.count(),&g);
    fmt::print("{} \n", loop.time());
  }
}
