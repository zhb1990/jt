// module jt:log.logger;
module jt;

import std;
import :detail.string;
import :detail.vector;
import :log.message;

namespace jt::log {

/**
 * 日志记录器实现类
 * 负责实际的日志记录逻辑，包括日志级别管理、Sink管理和异步处理
 */
class logger_impl {
 public:
  /**
   * 构造函数
   * @param service 日志服务引用
   * @param name 日志记录器名称
   * @param sinks Sink集合（将被移动）
   * @param async 是否异步模式
   * @note 此构造函数参数被标记为NOLINT以避免误报
   */
  logger_impl(service& service, const std::string_view& name,  // NOLINT
              detail::vector<service::sink_ptr>& sinks, const bool async)
      : service_(service),
        name_(name),
        sinks_(std::move(sinks)),
        async_(async) {}

  /**
   * 设置日志级别
   * @param lv 要设置的日志级别
   * @note 使用relaxed内存序因为此操作不需要与其他线程同步
   */
  void set_level(const level lv) noexcept {
    lv_.store(lv, std::memory_order::relaxed);
  }

  /**
   * 获取当前日志级别
   * @return 当前日志级别
   * @note 使用relaxed内存序因为此操作不需要与其他线程同步
   */
  [[nodiscard]] auto get_level() const noexcept -> level {
    return lv_.load(std::memory_order::relaxed);
  }

  /**
   * 获取日志记录器名称
   * @return 日志记录器名称
   */
  [[nodiscard]] auto get_name() const noexcept -> std::string_view {
    return name_;
  }

  /**
   * 检查是否为异步模式
   * @return 如果是异步模式返回true，否则返回false
   */
  [[nodiscard]] auto is_async() const noexcept -> bool { return async_; }

  /**
   * 获取关联的日志服务
   * @return 日志服务引用
   */
  [[nodiscard]] auto get_service() const noexcept -> service& {
    return service_;
  }

  /**
   * 后端日志处理（内部使用）
   * 将日志消息分发到所有Sink
   * @param msg 要处理的日志消息
   * @note 此函数被标记为NOLINT以避免关于空catch块的警告
   */
  void backend_log(const message& msg) const {
    for (const auto& sink : sinks_) {
      try {
        sink->log(msg);
      } catch (...) {
        // 忽略所有异常以防止日志失败导致程序崩溃
      }
    }
  }

  /**
   * 后端刷新处理（内部使用）
   * 刷新所有Sink
   * @note 此函数被标记为NOLINT以避免关于空catch块的警告
   */
  void backend_flush() const {
    for (const auto& sink : sinks_) {
      try {
        sink->flush();
      } catch (...) {
        // 忽略所有异常以防止日志失败导致程序崩溃
      }
    }
  }

 private:
  /** 日志服务引用 */
  service& service_;
  /** 日志记录器名称 */
  detail::string name_;
  /** Sink集合 */
  detail::vector<service::sink_ptr> sinks_;
  /** 日志级别（使用原子操作以支持线程安全） */
  std::atomic<level> lv_{level::trace};
  /** 是否异步模式 */
  bool async_;
};

/**
 * 日志记录器构造函数
 * @param service 日志服务引用
 * @param name 日志记录器名称
 * @param sinks Sink集合
 * @param async 是否异步模式
 * @note 此构造函数参数被标记为NOLINT以避免误报
 */
logger::logger(service& service, const std::string_view& name,  // NOLINT
               detail::vector<sink_ptr> sinks, bool async)
    : impl_(detail::make_unique<logger_impl>(service, name, sinks, async)) {}
/**
 * 日志记录器析构函数
 */
logger::~logger() noexcept = default;

// ReSharper disable once CppMemberFunctionMayBeConst
/**
 * 设置日志级别
 * @param lv 要设置的日志级别
 */
void logger::set_level(const level lv) noexcept { return impl_->set_level(lv); }

/**
 * 获取当前日志级别
 * @return 当前日志级别
 */
auto logger::get_level() const noexcept -> level { return impl_->get_level(); }

/**
 * 获取日志记录器名称
 * @return 日志记录器名称
 */
auto logger::get_name() const noexcept -> std::string_view {
  return impl_->get_name();
}

/**
 * 刷新日志记录器
 * @note 此函数被标记为NOLINT以避免关于成员函数可能被声明为const的警告
 */
void logger::flush() {  // NOLINT(*-convert-member-functions-to-static)
  if (!impl_->is_async()) {
    // 同步模式下直接刷新所有Sink
    return impl_->backend_flush();
  }

  // 异步模式下委托给服务执行刷新
  return impl_->get_service().flush(weak_from_this());
}

/**
 * 检查是否应该记录指定级别的日志
 * @param lv 要检查的日志级别
 * @return 如果该级别的日志应该被记录返回true，否则返回false
 * @note 比较日志级别的数值来确定是否应该记录
 */
auto logger::should_log(level lv) const noexcept -> bool {
  return static_cast<std::uint8_t>(lv) <=
         static_cast<std::uint8_t>(impl_->get_level());
}

/**
 * 记录日志
 * @param sid 服务ID
 * @param lv 日志级别
 * @param buf 日志内容缓冲区
 * @param source 源代码位置信息
 * @note 此函数参数被标记为NOLINT以避免误报
 */
void logger::log(std::uint32_t sid, level lv, detail::buffer_1k& buf,  // NOLINT
                 const std::source_location& source) {
  auto& service = impl_->get_service();
  if (!impl_->is_async()) {
    // 同步模式下直接处理日志消息
    message msg;
    msg.buf = std::move(buf);
    msg.source = source;
    msg.lv = lv;
    msg.sid = sid;
    msg.point = std::chrono::system_clock::now();
    msg.type = message_type::log;
    msg.tid = detail::tid();
    return impl_->backend_log(msg);
  }

  // 异步模式下委托给服务处理日志消息
  return service.log(weak_from_this(), sid, lv, buf, source);
}

// ReSharper disable once CppMemberFunctionMayBeConst
/**
 * 后端日志处理（内部使用）
 * @param msg 要处理的日志消息
 */
void logger::backend_log(const message& msg) { return impl_->backend_log(msg); }

// ReSharper disable once CppMemberFunctionMayBeConst
/**
 * 后端刷新处理（内部使用）
 */
void logger::backend_flush() { return impl_->backend_flush(); }

}  // namespace jt::log