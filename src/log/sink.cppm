module;

#include "../detail/config.h"

export module jt:log.sink;

import std;
import :detail.buffer;
import :detail.memory;
import :log.level;
import :log.fwd;

export namespace jt::log {

/**
 * 日志Sink的实现类前向声明
 */
class sink_impl;

/**
 * 日志Sink基类接口
 * 定义了所有日志Sink必须实现的接口
 */
class JT_API sink {
 public:
  /** 时间点类型别名 */
  using time_point = std::chrono::system_clock::time_point;
  /** 格式化器智能指针类型别名 */
  using formatter_ptr = detail::dynamic_unique_ptr<formatter>;

  /**
   * 构造函数
   * 创建一个Sink实例
   */
  sink();

  /**
   * 析构函数
   * 虚析构确保派生类正确析构
   */
  virtual ~sink() noexcept;

  /**
   * 设置日志级别
   * @param lv 要设置的日志级别
   */
  void set_level(level lv);

  /**
   * 处理日志消息
   * @param msg 要处理的日志消息
   */
  void log(const message& msg);

  /**
   * 刷新Sink
   * 确保所有缓冲的日志都被写出
   */
  void flush();

  /**
   * 设置格式化器
   * @param ptr 要设置的格式化器智能指针
   */
  void set_formatter(formatter_ptr ptr);

  /**
   * 写入日志数据（纯虚函数）
   * 必须由派生类实现
   * @param lv 日志级别
   * @param point 时间点
   * @param buf 日志缓冲区
   * @param color_start 颜色开始位置（用于控制台着色）
   * @param color_stop 颜色结束位置（用于控制台着色）
   */
  virtual void write(level lv, const time_point& point,
                     const detail::buffer_1k& buf, std::size_t color_start,
                     std::size_t color_stop) = 0;

  /**
   * 刷新解锁（纯虚函数）
   * 必须由派生类实现
   * 用于在不持有内部锁的情况下刷新
   */
  virtual void flush_unlock() = 0;

 private:
  /** Pimpl idiom实现指针 */
  detail::unique_ptr<sink_impl> impl_;
};

}  // namespace jt::log