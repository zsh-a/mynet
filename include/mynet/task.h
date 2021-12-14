#pragma once
#include <coroutine>

#include "mynet/common.h"
#include "mynet/resumable.h"
namespace mynet {
template <typename R = void>
struct Task : public Resumable {
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

    template <typename U>
    struct awaiter {
      bool await_ready() { return false; }
      template <typename P>
      void await_suspend(P) {
        if (!continuation_.handle_.done()) {
          continuation_.handle_.resume();
        }
      }
      U await_resume() { return continuation_.handle_.promise().get_result(); }
      Task<U> continuation_;
    };

    template <class Cont>
    auto await_transform(Cont continuation) {
      return awaiter{continuation};
    }
  };

  Task(const auto& handle) : handle_(handle) {}

  bool done() { return handle_.done(); }
  void resume() { handle_.resume(); }
  R& get_result() { return handle_.promise().value_; }

  //  private:
  coro_handle handle_;
};

}  // namespace mynet