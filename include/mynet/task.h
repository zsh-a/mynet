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
                loop_.run_immediately(cont);
            }
        }
        constexpr void await_resume() const noexcept {}
    };

    auto final_suspend() noexcept { return final_awaiter{}; }
    void return_value(R&& value) { value_.emplace(std::forward<R>(value));}
    void unhandled_exception() { std::terminate(); }
    // std::suspend_always yield_value(R&& value){
    //   value_.emplace(std::forward<R>(value));
    // }

    void resume() override{
      auto handle = coro_handle::from_promise(*this);
      handle.resume();
    }
    bool done() override{
      return coro_handle::from_promise(*this).done();
    }
    R&& get_result() { return std::move(value_).value(); }
    std::optional<R> value_{};

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
    Resumable* continuation_{nullptr};
  };

  struct Awaiter {
    bool await_ready() { return false; }
    
    template<typename Promise>
    void await_suspend(std::coroutine_handle<Promise> caller) {
      self_.promise().continuation_ = &caller.promise(); // save caller to continuation_
      loop.run_immediately(&self_.promise());
    }
    R&& await_resume() { return std::move(self_.promise().value_).value(); }

    coro_handle self_;
    EventLoop& loop{EventLoop::get()};
  };

  auto operator co_await(){
    
    return Awaiter{handle_};
  }

  Resumable* get_resumable() {
    return &handle_.promise();
  }

  Task(const auto& handle) : handle_(handle) {}

  ~Task() {
    if(handle_.done()) handle_.destroy();
  }

  bool done() { return handle_.done(); }
  void resume() { handle_.resume(); }
  R&& get_result() { return std::move(handle_.promise().value_).value(); }

  //  private:
  coro_handle handle_;
};

template <typename R = void>
struct NoWaitTask {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type : public Resumable {
    NoWaitTask get_return_object() { return NoWaitTask(coro_handle::from_promise(*this)); }

    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    // void return_value(R&& value) { value_ = std::forward<R>(value); }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
    
    void resume() override{
      auto handle = coro_handle::from_promise(*this);
      handle.resume();
    }
    bool done() override{
      return coro_handle::from_promise(*this).done();
    }

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
  return std::forward<Task>(task);
}

}  // namespace mynet