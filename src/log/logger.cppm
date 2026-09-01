module;

#include <memory>

#include "../detail/config.h"

export module jt:log.logger;

import std;
import :detail.memory;
import :detail.buffer;
import :detail.vector;
import :log.sink;
import :log.fwd;
import :log.record;

namespace jt::log {
class logger_impl;
class service_impl;
}  // namespace jt::log

export namespace jt::log {

/**
 * 日志记录器类
 * 负责管理日志Sink集合和日志级别，并将日志消息分发到各个Sink
 * 继承自enable_shared_from_this以支持shared_from_this()方法
 */
class logger : public std::enable_shared_from_this<logger> {
 public:
  /** Sink智能指针类型别名 */
  using sink_ptr = detail::dynamic_unique_ptr<sink>;

  /**
   * 仅允许 service::create_logger 通过 allocate_shared 构造。
   * 用户无法构造 ctor_key，因此不能直接创建 logger。
   */
  class ctor_key {
    ctor_key() = default;
    friend class service;
  };

  logger(ctor_key, std::shared_ptr<service_impl>& service,
         const std::string_view& name, detail::vector<sink_ptr> sinks,
         bool async);

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

 private:
  friend class service_impl;
  friend class service;

  void backend_log(const log_record_view& record);
  void backend_flush();

  /** Pimpl idiom实现指针 */
  detail::unique_ptr<logger_impl> impl_;
};

}  // namespace jt::log
