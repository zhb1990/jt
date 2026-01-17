module;

#include "../detail/config.h"

export module jt:log.sink;

import std;
import :detail.buffer;
import :detail.memory;
import :log.level;
import :log.fwd;

export namespace jt::log {

class sink_impl;

class JT_API sink {
 public:
  using time_point = std::chrono::system_clock::time_point;
  using formatter_ptr = detail::dynamic_unique_ptr<formatter>;

  sink();

  virtual ~sink() noexcept;

  void set_level(level lv);

  void log(const message& msg);

  void flush();

  void set_formatter(formatter_ptr ptr);

  virtual void write(level lv, const time_point& point,
                     const detail::buffer_1k& buf, std::size_t color_start,
                     std::size_t color_stop) = 0;

  virtual void flush_unlock() = 0;

 private:
  detail::unique_ptr<sink_impl> impl_;
};

}  // namespace jt::log