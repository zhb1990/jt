export module jt:log.message;

import std;
import :detail.buffer;
import :log.level;

export namespace jt::log {

class logger;

enum message_type : std::uint8_t { log, flush };

struct message {
  // ReSharper disable once CppRedundantQualifier
  message_type type{message_type::log};
  level lv{level::off};
  std::weak_ptr<logger> logger{};
  std::chrono::system_clock::time_point point{};
  std::source_location source{};
  detail::buffer_1k buf;
  std::uint32_t tid{0};
  std::uint32_t sid{0};

  std::atomic<void*> next{nullptr};
};

}  // namespace jt::log