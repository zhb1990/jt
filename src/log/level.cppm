export module jt:log.level;

import std;

export namespace jt::log {

/**
 * 日志级别枚举
 * 定义了日志系统支持的各种日志级别，按优先级从低到高排序
 */
enum class level : std::uint8_t {
  /** 关闭日志 */
  off = 0,
  /** 严重错误 - 系统不可用 */
  critical = 1,
  /** 错误 - 系统功能受影响 */
  error = 2,
  /** 警告 - 可能出现问题 */
  warn = 3,
  /** 信息 - 正常操作的信息 */
  info = 4,
  /** 调试 - 调试信息 */
  debug = 5,
  /** 跟踪 - 最详细的日志信息 */
  trace = 6
};

/**
 * 将日志级别转换为完整字符串表示
 * @param lv 日志级别
 * @return 对应的字符串表示，如果级别无效则返回空字符串
 */
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

/**
 * 将日志级别转换为简短字符串表示
 * @param lv 日志级别
 * @return 对应的单字符表示，如果级别无效则返回空字符串
 */
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
