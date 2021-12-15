
// #define CATCH_CONFIG_MAIN
#include <iostream>

#include "gtest/gtest.h"
#include "mynet/add.h"
#include "mynet/event_loop.h"
#include "mynet/sleep.h"
#include "mynet/task.h"
// #include "catch2/catch.hpp"

// TEST_CASE( "simple" )
// {
//     REQUIRE( add(1,2) == 3 );
// }

TEST(hello, world) {
  EXPECT_EQ(add(2, 2), 4);
  // std::cout << "hello\n";
}

TEST(epoll, timeout) {
  using namespace mynet;
  Epoller epoller;
  auto& loop = EventLoop::get();
  auto start = loop.time();
  epoller.poll(500);
  auto end = loop.time();
  EXPECT_TRUE(end - start >= 500);
}

using namespace mynet;

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

class TaskAwaitTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  // virtual void SetUp() will be called before each test is run.  You
  // should define it if you need to initialize the variables.
  // Otherwise, this can be skipped.
  void SetUp() override { result.clear(); }

  // virtual void TearDown() will be called after each test is run.
  // You should define it if there is cleanup work to do.  Otherwise,
  // you don't have to provide it.
  //
  // virtual void TearDown() {
  // }
  std::vector<int> result;
};

TEST_F(TaskAwaitTest, task) {
  std::vector<int> expected{1};
  auto g = coro1(result);
  g.resume();
  EXPECT_EQ(result, expected);
}

TEST_F(TaskAwaitTest, task2) {
  std::vector<int> expected{2,1,20};
  auto& loop = EventLoop::get();
  loop.run_until_done(coro2(result).get_resumable());
  EXPECT_EQ(result, expected);
}
