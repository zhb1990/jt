module;

#include "../detail/config.h"

export module jt:log.sink;

import std;
import :detail.buffer;
import :detail.memory;
import :log.formatter;
import :log.level;

export namespace jt::log {

class sink_impl;

class sink {
 public:
  using time_point = std::chrono::system_clock::time_point;

  JT_API sink();

  JT_API virtual ~sink() noexcept;

  JT_API void set_level(level lv);

  JT_API void log(const message& msg);

  JT_API void flush();

  JT_API void set_formatter(formatter_ptr ptr);

  virtual void write(const time_point& point, const detail::buffer_1k& buf,
                     std::size_t& color_start, std::size_t& color_stop) = 0;

  virtual void flush_unlock() = 0;

 private:
  detail::unique_ptr<sink_impl> impl_;
};

using sink_ptr = detail::dynamic_unique_ptr<sink>;

}  // namespace jt::log