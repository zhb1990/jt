module;

#include "../detail/config.h"

export module jt:log.logger;

import std;
import :log.sink;
import :detail.memory;
import :detail.buffer;
import :detail.vector;

export namespace jt::log {

class service;
class logger_impl;
class message;

class logger : public std::enable_shared_from_this<logger> {
 public:
  friend class service;

  JT_API ~logger() noexcept;

  JT_API void set_level(level lv) noexcept;

  JT_API auto get_level() const noexcept -> level;

  JT_API auto get_name() const noexcept -> std::string_view;

  JT_API void flush() const;

  JT_API auto should_log(level lv) const noexcept -> bool;

  template <typename... Args>
  void log(
      level lv, const std::format_string<Args...>& fmt, Args&&... args,
      std::source_location source = std::source_location::current()) const {
    if (!should_log(lv)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      log(lv, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void critical(
      const std::format_string<Args...>& fmt, Args&&... args,
      std::source_location source = std::source_location::current()) const {
    if (!should_log(level::critical)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(level::critical, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void error(
      const std::format_string<Args...>& fmt, Args&&... args,
      std::source_location source = std::source_location::current()) const {
    if (!should_log(level::error)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(level::error, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void warn(
      const std::format_string<Args...>& fmt, Args&&... args,
      std::source_location source = std::source_location::current()) const {
    if (!should_log(level::warn)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(level::warn, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void info(
      const std::format_string<Args...>& fmt, Args&&... args,
      std::source_location source = std::source_location::current()) const {
    if (!should_log(level::info)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(level::info, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void debug(
      const std::format_string<Args...>& fmt, Args&&... args,
      std::source_location source = std::source_location::current()) const {
    if (!should_log(level::debug)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(level::debug, buf, source);
    } catch (...) {
    }
  }

  template <typename... Args>
  void trace(
      const std::format_string<Args...>& fmt, Args&&... args,
      std::source_location source = std::source_location::current()) const {
    if (!should_log(level::trace)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), std::string_view(fmt),
                     std::make_format_args(std::forward<Args>(args)...));
      return log(level::trace, buf, source);
    } catch (...) {
    }
  }

 protected:
  logger(service& service, const std::string_view& name,
         detail::vector<sink_ptr> sinks, bool async);

  JT_API void log(level lv, detail::buffer_1k& buf,
                  const std::source_location& source) const;

  void backend_log(const message& msg) const;

  void backend_flush() const;

 private:
  detail::unique_ptr<logger_impl> impl_;
};

}  // namespace jt::log