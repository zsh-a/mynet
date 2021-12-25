#pragma once

namespace mynet {

struct Resumable {
  virtual void resume() = 0;
  virtual bool done() = 0;
  virtual ~Resumable() = default;
};

struct Func : public Resumable {
  std::function<void()> fun_;
  Func(const std::function<void()>& fun) : fun_(fun) {}
  void resume() override { fun_(); }
  bool done() override { return true; }
};

struct CoResumable : public Resumable {
  std::coroutine_handle<> handle_;
  CoResumable(std::coroutine_handle<> h) : handle_(h) {}
  void resume() override { handle_.resume(); }
  bool done() override { return handle_.done(); }

  // ~CoResumable() override {
  //   if(done()) handle_.destroy();
  // }
};

}  // namespace mynet