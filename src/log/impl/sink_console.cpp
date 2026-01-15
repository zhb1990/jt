module;

#include <cstdio>

// module jt:log.sink.console;
module jt;

import std;

namespace jt::log {

void sink_console::write(const time_point& point, const detail::buffer_1k& buf,
                         std::size_t& color_start, std::size_t& color_stop) {
  std::fwrite(buf.begin_read(), sizeof(char), buf.readable(), stdout);
}

void sink_console::flush_unlock() { std::fflush(stdout); }

}  // namespace jt::log