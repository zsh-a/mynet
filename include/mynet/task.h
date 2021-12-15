#pragma once
#include <coroutine>

#include "mynet/common.h"
#include "mynet/resumable.h"
#include "mynet/event_loop.h"
#include "fmt/format.h"
#include<optional>
namespace mynet {
template <typename R = void>
struct Task {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type : public Resumable{
    Task get_return_object() { return Task(coro_handle::from_promise(*this)); }

    std::suspend_always initial_suspend() { return {}; }
    struct final_awaiter {
        constexpr bool await_ready() const noexcept { return false; }
        template<typename Promise>
        constexpr void await_suspend(std::coroutine_handle<Promise> h) const noexcept {
            if (auto cont = h.promise().continuation_) {
                EventLoop& loop_{EventLoop::get()};
                loop_.run_immediately(std::make_unique<CoResumable>(cont));
            }
        }
        constexpr void await_resume() const noexcept {}
    };

    auto final_suspend() noexcept { return final_awaiter{}; }
    void return_value(R&& value) { value_.emplace(std::forward<R>(value));}
    void unhandled_exception() { std::terminate(); }

    void resume() override{
      auto handle = coro_handle::from_promise(*this);
      handle.resume();
    }
    bool done() override{
      return coro_handle::from_promise(*this).done();
    }

    R& get_result() { return value_; }
    std::optional<R> value_;

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
    std::coroutine_handle<> continuation_;
  };

  auto operator co_await(){
    struct Awaiter {
      bool await_ready() { return false; }
      void await_suspend(std::coroutine_handle<> caller) {
        self_.promise().continuation_ = caller; // save caller to continuation_
        loop.run_immediately(std::make_unique<CoResumable>(self_));
      }
      std::optional<R> await_resume() { return self_.promise().value_; }

      coro_handle self_;
      EventLoop& loop{EventLoop::get()};
    };
    return Awaiter{handle_};
  }

  std::unique_ptr<Resumable> get_resumable() {
    return std::make_unique<CoResumable>(handle_);
  }

  Task(const auto& handle) : handle_(handle) {}

  ~Task() {
    if(handle_.done()) handle_.destroy();
  }

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

template<typename Task>
auto create_task(Task&& task){
  auto& loop = EventLoop::get();
  loop.run_immediately(task.get_resumable());
  return task;
}

}  // namespace mynet