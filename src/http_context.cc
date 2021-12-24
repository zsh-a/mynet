#include "mynet/http_context.h"

namespace mynet {
namespace http {

std::unordered_map<std::string_view, HttpRequest::Method> HttpRequest::methods{
    {"GET", HttpRequest::Method::GET}, {"POST", HttpRequest::Method::POST}};
std::unordered_map<std::string_view, Version> HttpRequest::versions{
    {"HTTP/1.0", Version::HTTP10}, {"HTTP/1.1", Version::HTTP11}};
std::unordered_map<HttpResponse::StatusCode, std::string>
    HttpResponse::status_code_to_string = {
        {HttpResponse::StatusCode::k200Ok, "200 OK"},
};
std::unordered_map<Version, std::string> version_to_string{
    {Version::HTTP10, "HTTP/1.0"}, {Version::HTTP11, "HTTP/1.1"}};
}  // namespace http
}  // namespace mynet