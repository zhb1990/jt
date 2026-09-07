module;

#include "../detail/platform/config.h"

export module jt.log.format;

import std;
import jt.base.buffer;

export namespace jt::log {

// Append synchronously. The argument store and referenced values must remain
// alive for this call; asynchronous logging queues the resulting bytes only.
JT_API void vformat_to(base::buffer_1k& output, std::string_view fmt,
                       std::format_args args);

template <typename... Args>
void format_to(base::buffer_1k& output, std::format_string<Args...> fmt,
               Args&&... args) {
  jt::log::vformat_to(output, fmt.get(), std::make_format_args(args...));
}

// Replace the timestamp caches using the current zone, at second precision.
// Keep chrono formatter instantiations beside the runtime formatting engine.
JT_API void format_local_time(std::chrono::system_clock::time_point timestamp,
                              base::base_memory_buffer<128>& date_and_time,
                              base::base_memory_buffer<32>& zone_offset);

}  // namespace jt::log
