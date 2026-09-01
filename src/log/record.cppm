export module jt:log.record;

import std;
import :log.level;

export namespace jt::log {

struct log_record_view {
  level lv{level::off};
  std::uint32_t service_id{0};
  std::uint64_t thread_id{0};
  std::chrono::system_clock::time_point timestamp{};
  std::source_location source{};
  std::string_view payload{};
};

}  // namespace jt::log
