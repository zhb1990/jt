module;

#include "../detail/config.h"

export module jt:log.sink.console;

import std;
import :log.sink;
import :detail.memory;

export namespace jt::log {

/**
 * 控制台Sink实现类的前向声明
 */
class sink_console_impl;

/**
 * 标准输出Sink类
 * 将日志输出到stdout（标准输出），支持彩色输出
 * 使用final关键字防止被继承
 */
class JT_API sink_stdout final : public sink {
 public:
  /**
   * 构造函数
   * 创建一个标准输出Sink实例
   */
  sink_stdout();

  /**
   * 析构函数
   */
  ~sink_stdout() noexcept override;

  /**
   * 写入日志到标准输出
   * @param lv 日志级别
   * @param 时间点（未使用）
   * @param buf 日志缓冲区
   * @param color_start 颜色开始位置（用于控制台着色）
   * @param color_stop 颜色结束位置（用于控制台着色）
   */
  void write(level lv, const time_point&, const detail::buffer_1k& buf,
             std::size_t color_start, std::size_t color_stop) override;

  /**
   * 刷新标准输出缓冲区
   * 确保所有缓冲的数据都被输出
   */
  void flush_unlock() override;

 private:
  /** 控制台Sink实现类的引用 */
  sink_console_impl& impl_;
};

/**
 * 标准错误Sink类
 * 将日志输出到stderr（标准错误），支持彩色输出
 * 使用final关键字防止被继承
 */
class JT_API sink_stderr final : public sink {
 public:
  /**
   * 构造函数
   * 创建一个标准错误Sink实例
   */
  sink_stderr();

  /**
   * 析构函数
   */
  ~sink_stderr() noexcept override;

  /**
   * 写入日志到标准错误
   * @param lv 日志级别
   * @param 时间点（未使用）
   * @param buf 日志缓冲区
   * @param color_start 颜色开始位置（用于控制台着色）
   * @param color_stop 颜色结束位置（用于控制台着色）
   */
  void write(level lv, const time_point&, const detail::buffer_1k& buf,
             std::size_t color_start, std::size_t color_stop) override;

  /**
   * 刷新标准错误缓冲区
   * 确保所有缓冲的数据都被输出
   */
  void flush_unlock() override;

 private:
  /** 控制台Sink实现类的引用 */
  sink_console_impl& impl_;
};

/**
 * 写入数据到标准输出（信息级别）
 * @param buf 要输出的数据缓冲区
 */
JT_API void write_stdout(const detail::buffer_1k& buf);

/**
 * 写入数据到标准错误（错误级别）
 * @param buf 要输出的数据缓冲区
 */
JT_API void write_stderr(const detail::buffer_1k& buf);

/**
 * 打印格式化信息到标准错误
 * @tparam Args 格式化参数类型
 * @param fmt 格式化字符串
 * @param args 格式化参数
 * @note 此函数被包装在try-catch块中以防止格式化错误导致程序崩溃
 */
template <typename... Args>
void print_stderr(std::format_string<Args...> fmt, Args&&... args) {
  try {
    detail::buffer_1k buf;
    std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
    write_stderr(buf);
  } catch (...) {
    // 忽略所有异常以防止日志失败导致程序崩溃
  }
}

/**
 * 打印格式化信息到标准输出
 * @tparam Args 格式化参数类型
 * @param fmt 格式化字符串
 * @param args 格式化参数
 * @note 此函数被包装在try-catch块中以防止格式化错误导致程序崩溃
 */
template <typename... Args>
void print_stdout(std::format_string<Args...> fmt, Args&&... args) {
  try {
    detail::buffer_1k buf;
    std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
    write_stdout(buf);
  } catch (...) {
    // 忽略所有异常以防止日志失败导致程序崩溃
  }
}

}  // namespace jt::log