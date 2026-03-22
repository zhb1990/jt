module;

#include "../detail/config.h"

export module jt:log.logger;

import std;
import :detail.memory;
import :detail.buffer;
import :detail.vector;
import :log.sink;
import :log.fwd;

export namespace jt::log {

/**
 * 日志记录器实现类的前向声明
 */
class logger_impl;

/**
 * 日志记录器类
 * 负责管理日志Sink集合和日志级别，并将日志消息分发到各个Sink
 * 继承自enable_shared_from_this以支持shared_from_this()方法
 */
class logger : public std::enable_shared_from_this<logger> {
 public:
  /** 友元类：服务实现类，需要访问logger的内部成员 */
  friend class service_impl;
  /** Sink智能指针类型别名 */
  using sink_ptr = detail::dynamic_unique_ptr<sink>;

  /**
   * 构造函数
   * @param service 日志服务引用
   * @param name 日志记录器名称
   * @param sinks Sink集合
   * @param async 是否异步模式
   */
  logger(service& service, const std::string_view& name,
         detail::vector<sink_ptr> sinks, bool async);

  /**
   * 析构函数
   */
  JT_API ~logger() noexcept;

  /**
   * 设置日志级别
   * @param lv 要设置的日志级别
   */
  JT_API void set_level(level lv) noexcept;

  /**
   * 获取当前日志级别
   * @return 当前日志级别
   */
  [[nodiscard]] JT_API auto get_level() const noexcept -> level;

  /**
   * 获取日志记录器名称
   * @return 日志记录器名称
   */
  [[nodiscard]] JT_API auto get_name() const noexcept -> std::string_view;

  /**
   * 刷新日志记录器
   * 同步刷新所有Sink（在异步模式下会触发后台刷新）
   */
  JT_API void flush();

  /**
   * 检查是否应该记录指定级别的日志
   * @param lv 要检查的日志级别
   * @return 如果该级别的日志应该被记录返回true，否则返回false
   */
  [[nodiscard]] JT_API auto should_log(level lv) const noexcept -> bool;

  /**
   * 记录日志
   * @param sid 服务ID
   * @param lv 日志级别
   * @param buf 日志内容缓冲区
   * @param source 源代码位置信息
   */
  JT_API void log(std::uint32_t sid, level lv, detail::buffer_1k& buf,
                  const std::source_location& source);

 protected:
  /**
   * 后端日志处理（内部使用）
   * @param msg 要处理的日志消息
   */
  void backend_log(const message& msg);

  /**
   * 后端刷新处理（内部使用）
   */
  void backend_flush();

 private:
  /** Pimpl idiom实现指针 */
  detail::unique_ptr<logger_impl> impl_;
};

}  // namespace jt::log