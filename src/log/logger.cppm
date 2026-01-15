module;

#include "../detail/config.h"

export module jt:log.logger;

import std;
import :detail.memory;
import :detail.buffer;
import :detail.vector;
import :log.sink;
import :log.fwd;

export namespace jt::log {

class logger_impl;

class logger : public std::enable_shared_from_this<logger> {
 public:
  using sink_ptr = detail::dynamic_unique_ptr<sink>;
  logger(service& service, const std::string_view& name,
         detail::vector<sink_ptr> sinks, bool async);

  JT_API ~logger() noexcept;

  JT_API void set_level(level lv) noexcept;

  [[nodiscard]] JT_API auto get_level() const noexcept -> level;

  [[nodiscard]] JT_API auto get_name() const noexcept -> std::string_view;

  JT_API void flush();

  [[nodiscard]] JT_API auto should_log(level lv) const noexcept -> bool;

  JT_API void log(std::uint32_t sid, level lv, detail::buffer_1k& buf,
                  const std::source_location& source);

 protected:
  void backend_log(const message& msg);

  void backend_flush();

 private:
  detail::unique_ptr<logger_impl> impl_;
};

template <typename... Args>
struct log {
  log(const std::shared_ptr<logger>& logger, const level lv,
      std::format_string<Args...> fmt, Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(lv)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(0, lv, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
log(const std::shared_ptr<logger>& logger, level lv,
    std::format_string<Args...> fmt, Args&&... args) -> log<Args...>;

template <typename... Args>
struct critical {
  critical(
      const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::critical)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(0, level::critical, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
critical(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
         Args&&... args) -> critical<Args...>;

template <typename... Args>
struct error {
  error(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::error)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(0, level::error, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
error(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> error<Args...>;

template <typename... Args>
struct warn {
  warn(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
       Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::warn)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(0, level::warn, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
warn(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
     Args&&... args) -> warn<Args...>;

template <typename... Args>
struct info {
  info(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
       Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::info)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(0, level::info, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
info(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
     Args&&... args) -> info<Args...>;

template <typename... Args>
struct debug {
  debug(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::debug)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(0, level::debug, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
debug(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> debug<Args...>;

template <typename... Args>
struct trace {
  trace(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::trace)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(0, level::trace, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
trace(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> trace<Args...>;
}  // namespace jt::log