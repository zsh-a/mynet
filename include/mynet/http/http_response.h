#pragma once
#include <unordered_map>

#include "mynet/common.h"
#include "mynet/http/common.h"
namespace mynet {

struct HttpResponse {
  enum class StatusCode : uint16_t {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  Version version_;
  StatusCode status_code_;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
};

}  // namespace mynet
