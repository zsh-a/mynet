
// #define CATCH_CONFIG_MAIN
#include <iostream>
#include<chrono>
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
using namespace mynet;
EventLoop g_loop;

TEST(hello, world) {
  EXPECT_EQ(add(2, 2), 4);
  // std::cout << "hello\n";
}

TEST(epoll, timeout) {
  using namespace mynet;
  using namespace std::chrono;
  Epoller epoller;
  auto start = g_loop.time();
  epoller.poll(500);
  auto end = g_loop.time();
  EXPECT_TRUE(end - start >= milliseconds{500});
}

Task<bool> sleep2s(){
  co_await mynet::sleep(&g_loop,std::chrono::milliseconds(2000));
}

TEST(task, sleep) {
  using namespace mynet;
  auto start = g_loop.time();
  g_loop.create_task(sleep2s());
  g_loop.run();
  auto end = g_loop.time();
  EXPECT_TRUE(end - start >=  milliseconds{2000});
}

Task<int> many_resume(){
  co_await std::suspend_always{};
  co_await std::suspend_always{};
  fmt::print("~~~~~~~~~\n");
  co_return 0;
}

// TEST(task, many_resume) {
//   using namespace mynet;
//   auto& loop = EventLoop::get();
//   auto g = many_resume();
//   loop.run_until_done(many_resume().get_resumable());
//   // while(!g.done()) g.resume();
//   EXPECT_EQ(g.get_result(),0);
// }



struct Dummy {};
Task<Dummy> coro1(std::vector<int>& result) {
  result.push_back(1);
  co_return Dummy{};
}

Task<Dummy> coro2(std::vector<int>& result) {
  result.push_back(2);
  co_await coro1(result)(&g_loop);
  result.push_back(20);
  co_return Dummy{};
}

Task<Dummy> coro3(std::vector<int>& result) {
  result.push_back(3);
  co_await coro2(result)(&g_loop);
  result.push_back(30);
  co_return Dummy{};
}

Task<Dummy> coro4(std::vector<int>& result) {
  result.push_back(4);
  co_await coro3(result)(&g_loop);
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
  g_loop.run_until_done(coro2(result)(&g_loop).get_resumable());
  EXPECT_EQ(result, expected);
}

TEST_F(TaskAwaitTest, task3) {
  std::vector<int> expected{3,2,1,20,30};
  g_loop.run_until_done(coro3(result)(&g_loop).get_resumable());
  EXPECT_EQ(result, expected);
}

TEST_F(TaskAwaitTest, task4) {
  std::vector<int> expected{4,3,2,1,20,30,40};
  g_loop.run_until_done(coro4(result)(&g_loop).get_resumable());
  EXPECT_EQ(result, expected);
}

Task<int> mul(int a,int b){
  co_return a * b;
}

Task<int> get_await_result(){
  auto a = co_await mul(2,3)(&g_loop);
  auto b = co_await mul(4,5)(&g_loop);
  co_return a + b;
}

TEST(task, get_await_result) {
  auto g = get_await_result();
  g_loop.run_until_done(g.get_resumable());
  EXPECT_EQ(g.get_result(),26);
}

Task<int> fibo(int n){
  if(n <= 1) co_return n;
  auto x = co_await fibo(n - 1)(&g_loop);
  x += co_await fibo(n - 2)(&g_loop);
  co_return x;
}

TEST(task, fibo3) {
  auto& g = fibo(3)(&g_loop);
  g_loop.run_until_done(g.get_resumable());
  EXPECT_EQ(g.get_result(),2);
}

TEST(task, fibo7) {
  auto& g = fibo(8)(&g_loop);
  g_loop.run_until_done(g.get_resumable());
  EXPECT_EQ(g.get_result(),21);
}