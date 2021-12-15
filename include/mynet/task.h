#pragma once
#include <coroutine>

#include "mynet/common.h"
#include "mynet/resumable.h"
#include "mynet/event_loop.h"
namespace mynet {
template <typename R = void>
struct Task {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    Task get_return_object() { return Task(coro_handle::from_promise(*this)); }

    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(R&& value) { value_ = std::forward<R>(value); }
    void unhandled_exception() { std::terminate(); }

    R& get_result() { return value_; }
    R value_;

    // template <typename U>
    // struct awaiter {
    //   bool await_ready() { return false; }
    //   template <typename P>
    //   void await_suspend(P) {
    //     if (!continuation_.handle_.done()) {
    //       continuation_.handle_.resume();
    //     }
    //   }
    //   U await_resume() { return continuation_.handle_.promise().get_result();
    //   } Task<U> continuation_;
    // };

    // template <class Cont>
    // auto await_transform(Cont continuation) {
    //   return awaiter{continuation};
    // }
  };

  auto operator co_await(){
    struct Awaiter {
      bool await_ready() { return false; }
      void await_suspend(std::coroutine_handle<> caller) {
        struct CallAndReturn : public Resumable{
          coro_handle self_;
          std::coroutine_handle<> caller_;
          EventLoop& loop{EventLoop::get()};
          CallAndReturn(coro_handle self,std::coroutine_handle<> caller):
            self_(self),caller_(caller)
          {

          }
          void resume() override{
            self_.resume();
            loop.run_immediately(std::make_unique<CoResumable>(caller_)); 
          }
          bool done() override{
            return self_.done();
          }
          ~CallAndReturn()override{
            if(done()) self_.destroy();
          }
        };
        loop.run_immediately(std::make_unique<CallAndReturn>(self_,caller));
      }
      R await_resume() { return R{}; }

      coro_handle self_;
      EventLoop& loop{EventLoop::get()};
    };
    return Awaiter{handle_};
  }

  std::unique_ptr<Resumable> get_resumable() {
    return std::make_unique<CoResumable>(handle_);
  }

  Task(const auto& handle) : handle_(handle) {}

  bool done() { return handle_.done(); }
  void resume() { handle_.resume(); }
  R& get_result() { return handle_.promise().value_; }

  //  private:
  coro_handle handle_;
};

template <typename R = void>
struct NoWaitTask {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    NoWaitTask get_return_object() { return NoWaitTask(coro_handle::from_promise(*this)); }

    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    // void return_value(R&& value) { value_ = std::forward<R>(value); }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
  };

  std::unique_ptr<Resumable> get_resumable() {
    return std::make_unique<CoResumable>(handle_);
  }

  NoWaitTask(const auto& handle) : handle_(handle) {}

  bool done() { return handle_.done(); }
  void resume() { handle_.resume(); }

  //  private:
  coro_handle handle_;
};

}  // namespace mynet