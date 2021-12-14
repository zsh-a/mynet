#pragma once
#include <coroutine>
#include <iostream>
#include <utility>

#include "mynet/common.h"

namespace mynet {

template <typename T, typename G>
struct PromiseBase {
  T value_;
  std::suspend_always yield_value(T value) {
    value_ = std::move(value);
    return {};
  }

  G get_return_object() { return G{this}; };

  std::suspend_never initial_suspend() { return {}; }
  std::suspend_always final_suspend() noexcept { return {}; }
  void return_void() {}
  void unhandled_exception() { std::terminate(); }
  static auto get_return_object_on_allocation_failure() { return G{nullptr}; }
};

template <typename T>
struct Generator : NonCopyable {
  using promise_type = PromiseBase<T, Generator>;
  using coro_handle = std::coroutine_handle<promise_type>;

  ~Generator() {
    if (handle_) {
      handle_.destroy();
    }
  }
  T operator()() { return std::move(handle_.promise().value_); }
  bool done() { return handle_.done(); }
  void resume() { return handle_.resume(); }
  //  private:
  explicit Generator(promise_type* promise) {
    handle_ = coro_handle::from_promise(*promise);
  }

  coro_handle handle_;
};

}  // namespace mynet