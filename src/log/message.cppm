module jt:log.message;

import std;
import :detail.buffer;
import :log.record;
import :log.fwd;

namespace jt::log {

enum class message_type : std::uint8_t {
  log,
  flush,
};

struct message {
  message_type type{message_type::log};
  std::weak_ptr<logger> target{};
  level lv{level::off};
  std::uint32_t service_id{0};
  std::uint64_t thread_id{0};
  std::chrono::system_clock::time_point timestamp{};
  std::source_location source{};
  detail::buffer_1k payload{};
  std::atomic<message*> next{nullptr};

  [[nodiscard]] auto record() const noexcept -> log_record_view {
    return {
        .lv = lv,
        .service_id = service_id,
        .thread_id = thread_id,
        .timestamp = timestamp,
        .source = source,
        .payload = std::string_view(payload),
    };
  }
};

}  // namespace jt::log
