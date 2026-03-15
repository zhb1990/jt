// module jt:log.sink;
module jt;

import std;
import :detail.cache_line;
import :log.message;
import :log.default_formatter;

namespace jt::log {

/**
 * 日志Sink的实现类
 * 负责实际的日志写入操作，包括日志级别过滤、格式化和输出
 */
class sink_impl {
 public:
  /**
   * 构造函数
   * 创建默认格式化器
   * @note 此构造函数被标记为NOLINT以避免关于成员函数可能被声明为static的警告
   */
  sink_impl() {  // NOLINT
    formatter_ = detail::make_dynamic_unique<formatter, default_formatter>();
  }

  /**
   * 设置日志级别
   * @param lv 要设置的日志级别
   * @note 使用relaxed内存序因为此操作不需要与其他线程同步
   */
  void set_level(const level lv) { lv_.store(lv, std::memory_order::relaxed); }

  /**
   * 处理日志消息
   * @param msg 要处理的日志消息
   * @param s 用于写入日志的Sink接口指针
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  void log(const message& msg, sink* s) {  // NOLINT
    // 检查日志级别是否满足阈值
    if (static_cast<std::uint8_t>(msg.lv) >
        static_cast<std::uint8_t>(lv_.load(std::memory_order::relaxed))) {
      return;
    }

    detail::buffer_1k buf;
    std::size_t color_start, color_stop;
    std::lock_guard lock(mtx_);
    // 使用格式化器格式化日志消息
    formatter_->format(msg, buf, color_start, color_stop);
    // 写入格式化后的日志
    return s->write(msg.lv, msg.point, buf, color_start, color_stop);
  }

  /**
   * 刷新Sink
   * @param s 用于刷新的Sink接口指针
   * @note 此函数被标记为NOLINT以避免关于成员函数可能被声明为static的警告
   */
  void flush(sink* s) {  // NOLINT(*-convert-member-functions-to-static)
    std::lock_guard lock(mtx_);
    return s->flush_unlock();
  }

  /**
   * 设置格式化器
   * @param ptr 要设置的格式化器智能指针
   */
  void set_formatter(sink::formatter_ptr ptr) {
    std::lock_guard lock(mtx_);
    formatter_ = std::move(ptr);
  }

 private:
  /** 日志级别（使用原子操作以支持线程安全） */
  std::atomic<level> lv_{level::trace};
  /** 填充字节以确保结构体大小为缓存行的整数倍，避免伪共享 */
  char padding[detail::cache_line_bytes - sizeof(std::atomic<level>)];

  /** 格式化器智能指针 */
  sink::formatter_ptr formatter_;
  /** 互斥锁，用于保护对formatter_的访问 */
  std::mutex mtx_;
};

/**
 * Sink构造函数
 * @note 此构造函数被标记为NOLINT以避免误报
 */
sink::sink() { impl_ = detail::make_unique<sink_impl>(); }  // NOLINT

/**
 * Sink析构函数
 */
sink::~sink() noexcept = default;

// ReSharper disable once CppMemberFunctionMayBeConst
/**
 * 设置日志级别
 * @param lv 要设置的日志级别
 */
void sink::set_level(const level lv) { return impl_->set_level(lv); }

/**
 * 处理日志消息
 * @param msg 要处理的日志消息
 */
void sink::log(const message& msg) { return impl_->log(msg, this); }

/**
 * 刷新Sink
 */
void sink::flush() { return impl_->flush(this); }

// ReSharper disable once CppMemberFunctionMayBeConst
/**
 * 设置格式化器
 * @param ptr 要设置的格式化器智能指针
 */
void sink::set_formatter(formatter_ptr ptr) {
  return impl_->set_formatter(std::move(ptr));
}

}  // namespace jt::log