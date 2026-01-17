module;

#include "../detail/config.h"

export module jt:log.sink.console;

import std;
import :log.sink;
import :detail.memory;

export namespace jt::log {

class sink_console_impl;

class JT_API sink_stdout final : public sink {
 public:
  sink_stdout();

  ~sink_stdout() noexcept override;

  void write(level lv, const time_point& point, const detail::buffer_1k& buf,
             std::size_t color_start, std::size_t color_stop) override;

  void flush_unlock() override;

 private:
  sink_console_impl& impl_;
};

class JT_API sink_stderr final : public sink {
 public:
  sink_stderr();

  ~sink_stderr() noexcept override;

  void write(level lv, const time_point& point, const detail::buffer_1k& buf,
             std::size_t color_start, std::size_t color_stop) override;

  void flush_unlock() override;

 private:
  sink_console_impl& impl_;
};

}  // namespace jt::log