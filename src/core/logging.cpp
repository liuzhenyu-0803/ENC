#include "core/logging.h"

#include <iostream>

namespace navscene::internal {

void Log(LogLevel level, std::string_view message) {
  const char* level_name = "INFO";
  switch (level) {
    case LogLevel::kInfo:
      level_name = "INFO";
      break;
    case LogLevel::kWarning:
      level_name = "WARN";
      break;
    case LogLevel::kError:
      level_name = "ERROR";
      break;
  }

  std::clog << "[navscene][" << level_name << "] " << message << '\n';
}

}  // namespace navscene::internal
