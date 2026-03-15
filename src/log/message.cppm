export module jt:log.message;

import std;
import :detail.buffer;
import :log.level;
import :log.fwd;

export namespace jt::log {

/**
 * 日志消息类型枚举
 */
enum message_type : std::uint8_t { log, flush };

/**
 * 日志消息结构体
 * 用于在日志系统内部传递日志信息
 */
struct message {
  // ReSharper disable once CppRedundantQualifier
  /** 消息类型：日志或刷新 */
  message_type type{message_type::log};
  /** 日志级别 */
  level lv{level::off};
  /** 服务ID */
  std::uint32_t sid{0};
  /** 线程ID */
  std::uint64_t tid{0};
  /** 日志记录器弱引用（避免循环引用） */
  std::weak_ptr<logger> logger{};
  /** 时间戳 */
  std::chrono::system_clock::time_point point{};
  /** 源代码位置信息（文件、行号等） */
  std::source_location source{};
  /** 日志内容缓冲区 */
  detail::buffer_1k buf;

  /** 下一个消息的指针（用于链表或队列） */
  std::atomic<void*> next{nullptr};
};

}  // namespace jt::log