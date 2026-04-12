#pragma once

#include <string_view>

namespace navscene::internal {

enum class LogLevel {
  kInfo = 0,
  kWarning,
  kError,
};

void Log(LogLevel level, std::string_view message);

}  // namespace navscene::internal
