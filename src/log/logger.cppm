module;

#include "../detail/config.h"

export module jt:log.logger;

import std;
import :detail.memory;
import :detail.buffer;
import :detail.vector;
import :log.fwd;
import :log.sink;

namespace jt::log {

class logger_impl;

export class logger : public std::enable_shared_from_this<logger> {
 public:
  friend class service;

  JT_API ~logger() noexcept;

  JT_API void set_level(level lv) noexcept;

  [[nodiscard]] JT_API auto get_level() const noexcept -> level;

  [[nodiscard]] JT_API auto get_name() const noexcept -> std::string_view;

  JT_API void flush();

  [[nodiscard]] JT_API auto should_log(level lv) const noexcept -> bool;

  template <typename... Args>
  void log(
      const level lv, const std::format_string<Args...>& fmt, Args&&... args,
      const std::source_location source = std::source_location::current()) {
    if (!should_log(lv)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      log(0, lv, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void critical(
      const std::format_string<Args...>& fmt, Args&&... args,
      const std::source_location source = std::source_location::current()) {
    if (!should_log(level::critical)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(0, level::critical, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void error(
      const std::format_string<Args...>& fmt, Args&&... args,
      const std::source_location source = std::source_location::current()) {
    if (!should_log(level::error)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(0, level::error, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void warn(
      const std::format_string<Args...>& fmt, Args&&... args,
      const std::source_location source = std::source_location::current()) {
    if (!should_log(level::warn)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(0, level::warn, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void info(
      const std::format_string<Args...>& fmt, Args&&... args,
      const std::source_location source = std::source_location::current()) {
    if (!should_log(level::info)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(0, level::info, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void debug(
      const std::format_string<Args...>& fmt, Args&&... args,
      const std::source_location source = std::source_location::current()) {
    if (!should_log(level::debug)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(0, level::debug, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void trace(
      const std::format_string<Args...>& fmt, Args&&... args,
      const std::source_location source = std::source_location::current()) {
    if (!should_log(level::trace)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(0, level::trace, buf, source);
    } catch (...) {
    }
  }

 protected:
  logger(service& service, const std::string_view& name,
         detail::vector<sink_ptr> sinks, bool async);

  JT_API void log(std::uint32_t sid, level lv, detail::buffer_1k& buf,
                  const std::source_location& source);

  void backend_log(const message& msg);

  void backend_flush();

 private:
  detail::unique_ptr<logger_impl> impl_;
};

}  // namespace jt::log