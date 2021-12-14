#pragma once

namespace mynet {

struct Resumable {
  virtual void resume() = 0;
  virtual bool done() = 0;
  virtual ~Resumable() = default;
};

}  // namespace mynet