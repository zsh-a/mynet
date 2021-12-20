#pragma once
#include <unordered_map>

#include "mynet/common.h"
#include "mynet/http/common.h"
namespace mynet {

struct HttpRequest {
  enum class Method {
    GET,
    POST,
  };

  Method method_;
  Version version_;
  std::string url_;
  std::unordered_map<std::string, std::string> headers_;
};

}  // namespace mynet
