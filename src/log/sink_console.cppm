module;

#include "../detail/config.h"

export module jt:log.sink.console;

import std;
import :log.sink;

export namespace jt::log {

class JT_API sink_console : public sink {
 public:
  void write(const time_point& point, const detail::buffer_1k& buf,
             std::size_t& color_start, std::size_t& color_stop) override;

  void flush_unlock() override;
};

}  // namespace jt::log