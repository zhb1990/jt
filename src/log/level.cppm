export module jt:log.level;

import std;

export namespace jt::log {

enum class level : std::uint8_t {
  off = 0,
  critical = 1,
  error = 2,
  warn = 3,
  info = 4,
  debug = 5,
  trace = 6
};

constexpr std::string_view to_string_view(const level lv) noexcept {
  switch (lv) {
    case level::critical:
      return "critical";
    case level::error:
      return "error";
    case level::warn:
      return "warn";
    case level::info:
      return "info";
    case level::debug:
      return "debug";
    case level::trace:
      return "trace";
    default:
      return "";
  }
}

constexpr std::string_view to_string_view_short(const level lv) noexcept {
  switch (lv) {
    case level::critical:
      return "C";
    case level::error:
      return "E";
    case level::warn:
      return "W";
    case level::info:
      return "I";
    case level::debug:
      return "D";
    case level::trace:
      return "T";
    default:
      return "";
  }
}

}  // namespace jt::log
