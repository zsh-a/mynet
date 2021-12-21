#pragma once
#include <source_location>

#include "fmt/chrono.h"
#include "fmt/format.h"
namespace mynet {

namespace log {

enum Level { Error, Warning, Info };
constexpr std::string_view LEVEL_NAME[] = {"error", "warning", "info"};

extern Level g_level;

struct Format {
  std::string_view fmt;
  std::source_location loc;

  // #A Constructor allowing implicit conversion
  Format(const char* _fmt,
         std::source_location _loc = std::source_location::current())
      : fmt{_fmt}, loc{_loc} {}
};

void Log(Level level, Format fmt, const auto&... args) {
  if (level <= g_level)
    fmt::print("{} {}\n",
               fmt::format("[{}]:{:%y-%m-%d %H:%M:%S}:{}:{}: ",
                           LEVEL_NAME[static_cast<unsigned int>(level)],
                           fmt::gmtime(std::time(nullptr)), fmt.loc.file_name(),
                           fmt.loc.line()),
               fmt::vformat(fmt.fmt, fmt::v8::make_format_args(args...)));
}

// void Log_Info(Format fmt, const auto&... args) {
//   Log(Info,fmt,args...);
// }

}  // namespace log
}  // namespace mynet