#pragma once
#include <iostream>
#include <map>

#include "mynet/common.h"
#include "mynet/connection.h"
namespace mynet {

namespace http {
enum class Version {
  HTTP10,
  HTTP11,
};

extern std::unordered_map<Version, std::string> version_to_string;

struct HttpRequest {
  enum class Method {
    GET,
    POST,
  };
  static std::unordered_map<std::string_view, HttpRequest::Method> methods;

  static std::unordered_map<std::string_view, Version> versions;

  Method method_;
  Version version_;
  std::string url_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

struct HttpResponse {
  enum class StatusCode : uint16_t {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  constexpr static auto error_template =
      "<html>"
      "<head><title>{0} {1}</title></head>"
      "<body>"
      "<center><h1>{0} {1}</h1></center>"
      "<hr><center>mynet</center>"
      "</body>"
      "</html>";

  static std::unordered_map<StatusCode, std::string> status_code_to_string;

  Version version_;
  StatusCode status_code_;
  std::map<std::string, std::string> headers_;
  int content_length{0};
  std::string body_;

  HttpResponse() : version_(Version::HTTP11), status_code_(StatusCode::k200Ok) {
    headers_["Server"] = "mynet";
    auto const t = std::time(nullptr);
    auto tm = *std::gmtime(&t);
    char buf[128]{};
    std::strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    headers_["Date"] = fmt::format("{}", buf);
  }

  static HttpResponse get_error_resp(int code, const std::string &msg) {
    HttpResponse resp{};
    auto body = fmt::format(error_template, code, msg);
    resp.set_body(body);
    return resp;
  }

  void set_body(std::string s) {
    body_ = std::move(s);
    content_length = body_.size();
    headers_["Content-Type"] = "text/html; charset=utf-8";
    headers_["Content-Length"] = std::to_string(content_length);
  }

  std::string to_string() {
    // s.reserve(1024);
    std::string s;
    s.reserve(1024);
    s += fmt::format("{} {}\r\n", version_to_string[version_],
                     status_code_to_string[status_code_]);
    for (auto &&[k, v] : headers_) {
      s += fmt::format("{}: {}\r\n", k, v);
    }
    s += "\r\n" + body_;
    return s;
  }

  Connection::Buffer to_buffer() {
    Connection::Buffer buf;
    buf.reserve(1024);
    auto s = fmt::format("{} {}\r\n", version_to_string[version_],
                         status_code_to_string[status_code_]);
    buf.insert(buf.end(), s.begin(), s.end());
    for (auto &&[k, v] : headers_) {
      s = fmt::format("{}: {}\r\n", k, v);
      buf.insert(buf.end(), s.begin(), s.end());
    }
    buf.push_back('\r');
    buf.push_back('\n');
    buf.insert(buf.end(), body_.begin(), body_.end());
    return buf;
  }
};

enum class ParseState {
  PARSE_OK,
  PARSE_URL,
  PARSE_HEADERS,
  PARSE_BODY,
  PARSE_ERROR,
  PARSE_AGAIN,
};

enum class ParseURLSTate {
  PARSE_URL_OK,
  PARSE_URI_AGAIN,
  PARSE_URI_ERROR,
};

enum class ParseHeaderState {
  PARSE_HEADER_OK,
  PARSE_HEADER_AGAIN,
  PARSE_HEADER_ERROR
};

enum class ParseBodyState { PARSE_BODY_OK, PARSE_BODY_AGAIN, PARSE_BODY_ERROR };
class HttpContext {
  Connection::Buffer inbuffer_;

  ParseState state_{ParseState::PARSE_URL};
  HttpRequest req_;

  int content_length = 0;
  int cur_idx_ = 0;
  int line_start_ = 0;

  ParseURLSTate parse_url() {
    auto pos =
        std::find(inbuffer_.begin() + line_start_, inbuffer_.end(), '\r');
    if (pos == inbuffer_.end()) {
      return ParseURLSTate::PARSE_URI_AGAIN;
    }
    // input.erase(input.begin(),pos);
    int line_end = pos - inbuffer_.begin();
    std::string_view line(inbuffer_.begin() + line_start_,
                          inbuffer_.begin() + line_end);
    line_start_ = line_end + 2;

    int last = 0, idx = 0;
    while (idx < line.size() && line[idx] != ' ') ++idx;

    auto it = HttpRequest::methods.find(line.substr(last, idx - last));

    if (it == HttpRequest::methods.end()) {
      return ParseURLSTate::PARSE_URI_ERROR;
    }

    req_.method_ = it->second;

    // space
    while (idx < line.size() && isspace(line[idx])) ++idx;
    last = idx;

    // url
    while (idx < line.size() && line[idx] != ' ') ++idx;
    req_.url_ = line.substr(last, idx - last);

    last = idx;
    // space
    while (idx < line.size() && isspace(line[idx])) ++idx;
    last = idx;

    // version
    if (auto it = HttpRequest::versions.find(line.substr(idx, -1));
        it == HttpRequest::versions.end()) {
      return ParseURLSTate::PARSE_URI_ERROR;
    } else {
      req_.version_ = it->second;
    }
    return ParseURLSTate::PARSE_URL_OK;
  };

  ParseHeaderState parse_headers() {
    for (;;) {
      auto pos =
          std::find(inbuffer_.begin() + line_start_, inbuffer_.end(), '\r');
      if (pos == inbuffer_.end()) {
        return ParseHeaderState::PARSE_HEADER_AGAIN;
      } else if (pos - inbuffer_.begin() == line_start_) {
        inbuffer_.clear();
        line_start_ = 0;
        return ParseHeaderState::PARSE_HEADER_OK;
      }
      int line_end = pos - inbuffer_.begin();
      std::string_view line(inbuffer_.begin() + line_start_,
                            inbuffer_.begin() + line_end);
      line_start_ = line_end + 2;
      int last = 0, idx = 0;
      // key
      while (idx < line.size() && line[idx] != ':') ++idx;
      std::string_view key = line.substr(last, idx - last);
      ++idx;
      // space
      while (idx < line.size() && line[idx] == ' ') ++idx;
      last = idx;

      // value
      while (idx < line.size() && line[idx] != '\r') ++idx;
      std::string_view value = line.substr(last, idx - last);

      req_.headers_[std::string(key)] = value;
    }
  }

  ParseBodyState parse_body() {
    if (inbuffer_.size() < content_length) {
      return ParseBodyState::PARSE_BODY_AGAIN;
    }
    return ParseBodyState::PARSE_BODY_OK;
  }
  ParseState parse() {
    while (state_ != ParseState::PARSE_OK &&
           state_ != ParseState::PARSE_ERROR) {
      switch (state_) {
        case ParseState::PARSE_URL: {
          auto ret = parse_url();

          if (ret == ParseURLSTate::PARSE_URL_OK) {
            state_ = ParseState::PARSE_HEADERS;
          } else if (ret == ParseURLSTate::PARSE_URI_ERROR) {
            return ParseState::PARSE_ERROR;
          }
        } break;
        case ParseState::PARSE_HEADERS: {
          auto ret = parse_headers();
          if (ret == ParseHeaderState::PARSE_HEADER_OK) {
            state_ = ParseState::PARSE_BODY;
            if (auto it = req_.headers_.find("Content-Length");
                it != req_.headers_.end()) {
              content_length = std::stoi(it->second);
            }
          } else if (ret == ParseHeaderState::PARSE_HEADER_ERROR) {
            return ParseState::PARSE_ERROR;
          }
        } break;
        case ParseState::PARSE_BODY: {
          auto ret = parse_body();
          if (ret == ParseBodyState::PARSE_BODY_OK) {
            req_.body_ = std::string(inbuffer_.begin(),
                                     inbuffer_.begin() + content_length);
            inbuffer_.erase(inbuffer_.begin(),
                            inbuffer_.begin() + content_length);
            line_start_ = 0;
            return ParseState::PARSE_OK;
          } else if (ret == ParseBodyState::PARSE_BODY_ERROR) {
            return ParseState::PARSE_ERROR;
          }
        } break;
        default:
          return ParseState::PARSE_ERROR;
      }
    }
    return ParseState::PARSE_AGAIN;
  }
  void reset_state() {
    content_length = 0;
    line_start_ = 0;
    state_ = ParseState::PARSE_URL;
    std::exchange(req_, {});
  }

 public:
  template <typename HttpHandler>
  Task<bool> process_http(const Connection::Ptr &conn, HttpHandler &&handler) {
    Connection::Buffer buf;
    buf.reserve(4 * 1024);
    while (1) {
      buf.clear();
      ssize_t n = co_await conn->read(buf).run_in(conn->loop_);
      if (n <= 0) break;

      inbuffer_.insert(inbuffer_.end(), buf.begin(), buf.begin() + n);
      auto ret = parse();
      // auto ret = ParseState::PARSE_OK;
      if (ret == ParseState::PARSE_OK) {
        auto resp = co_await handler(req_).run_in(conn->loop_);
        co_await conn->write(resp.to_buffer()).run_in(conn->loop_);
        // auto s = resp.to_buffer();
        // fmt::print("{}\n", std::string(s.begin(), s.end()));
      } else {
        auto resp = HttpResponse::get_error_resp(500, "Error");
        // auto s = resp.to_buffer();
        co_await conn->write(resp.to_string()).run_in(conn->loop_);
      }
      reset_state();
    }
    conn->close();
    co_return true;
  }
};

}  // namespace http

}  // namespace mynet
