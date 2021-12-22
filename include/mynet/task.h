#pragma once
#include <coroutine>
#include <optional>

#include "fmt/format.h"
#include "mynet/common.h"
#include "mynet/event_loop.h"
#include "mynet/resumable.h"
namespace mynet {
template <typename R = void>
struct Task {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type : public Resumable {
    Task get_return_object() { return Task(coro_handle::from_promise(*this)); }

    std::suspend_always initial_suspend() { return {}; }
    struct final_awaiter {
      constexpr bool await_ready() const noexcept { return false; }
      template <typename Promise>
      constexpr void await_suspend(
          std::coroutine_handle<Promise> h) const noexcept {
        if (auto cont = h.promise().continuation_) {
          loop_->run_immediately(cont);
        }
      }
      constexpr void await_resume() const noexcept {}
      EventLoop* loop_;
    };

    auto final_suspend() noexcept { return final_awaiter{loop_}; }
    void return_value(R&& value) { value_.emplace(std::forward<R>(value)); }
    void unhandled_exception() { std::terminate(); }
    // std::suspend_always yield_value(R&& value){
    //   value_.emplace(std::forward<R>(value));
    // }

    void resume() override {
      auto handle = coro_handle::from_promise(*this);
      handle.resume();
    }
    bool done() override { return coro_handle::from_promise(*this).done(); }
    R&& get_result() { return std::move(value_).value(); }

    EventLoop* get_loop() { return loop_; }
    std::optional<R> value_{};
    Resumable* continuation_{nullptr};
    EventLoop* loop_{nullptr};
  };

  struct Awaiter {
    bool await_ready() { return false; }

    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> caller) {
      self_.promise().continuation_ =
          &caller.promise();  // save caller to continuation_
      loop_->run_immediately(&self_.promise());
    }
    R&& await_resume() { return std::move(self_.promise().value_).value(); }

    coro_handle self_;
    EventLoop* loop_;
  };

  auto operator co_await() { return Awaiter{handle_}; }

  Resumable* get_resumable() { return &handle_.promise(); }

  // Task(const auto& handle) : handle_(handle) {}

  ~Task() {
    if (handle_.done()) handle_.destroy();
  }

  // Task(Task&& other) noexcept: handle_(std::exchange(other.handle_, {})) {
  //   printf("aaaa\n");
  // }

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
    NoWaitTask get_return_object() {
      return NoWaitTask(coro_handle::from_promise(*this));
    }

    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    // void return_value(R&& value) { value_ = std::forward<R>(value); }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }

    void resume() override {
      auto handle = coro_handle::from_promise(*this);
      handle.resume();
    }
    bool done() override { return coro_handle::from_promise(*this).done(); }
  };

  Resumable* get_resumable() { return &handle_.promise(); }

  NoWaitTask(const auto& handle) : handle_(handle) {}

  bool done() { return handle_.done(); }
  void resume() { handle_.resume(); }

  //  private:
  coro_handle handle_;
};

}  // namespace mynet