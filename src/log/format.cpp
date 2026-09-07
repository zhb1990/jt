module jt.log.format;

import std;
import jt.base.buffer;

namespace jt::log {

// GCC/MinGW import std can emit strong definitions for libstdc++ format's
// function-local statics. Instantiate the runtime engine in this unit only.
void vformat_to(base::buffer_1k& output, std::string_view fmt,
                std::format_args args) {
  std::vformat_to(std::back_inserter(output), fmt, args);
}

void format_local_time(std::chrono::system_clock::time_point timestamp,
                       base::base_memory_buffer<128>& date_and_time,
                       base::base_memory_buffer<32>& zone_offset) {
  date_and_time.clear();
  const auto local_time = std::chrono::zoned_time{
      std::chrono::current_zone(),
      std::chrono::floor<std::chrono::seconds>(timestamp)};
  std::format_to(std::back_inserter(date_and_time), "{:%Y-%m-%d %H:%M:%S}",
                 local_time);
  zone_offset.clear();
  std::format_to(std::back_inserter(zone_offset), "{:%Ez}", local_time);
}

}  // namespace jt::log
