#pragma once
#include <cstdint>
namespace mynet {
struct Event {
  int fd;
  uint32_t events;
};

}  // namespace mynet