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
  friend class service_impl;
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

template <typename... Args>
struct vcritical {
  vcritical(
      const std::shared_ptr<logger>& logger, const std::string_view fmt,
      Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::critical)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(0, level::critical, buf, source);
    } catch (...) {
    }
  }

  vcritical(
      const std::shared_ptr<logger>& logger, const std::u8string_view fmt,
      Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::critical)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(0, level::critical, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
vcritical(const std::shared_ptr<logger>& logger, std::string_view fmt,
          Args&&... args) -> vcritical<Args...>;

template <typename... Args>
vcritical(const std::shared_ptr<logger>& logger, std::u8string_view fmt,
          Args&&... args) -> vcritical<Args...>;

template <typename... Args>
struct verror {
  verror(const std::shared_ptr<logger>& logger, const std::string_view fmt,
         Args&&... args,
         const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::error)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(0, level::error, buf, source);
    } catch (...) {
    }
  }

  verror(const std::shared_ptr<logger>& logger, const std::u8string_view fmt,
         Args&&... args,
         const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::error)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(0, level::error, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
verror(const std::shared_ptr<logger>& logger, std::string_view fmt,
       Args&&... args) -> verror<Args...>;

template <typename... Args>
verror(const std::shared_ptr<logger>& logger, std::u8string_view fmt,
       Args&&... args) -> verror<Args...>;

template <typename... Args>
struct vwarn {
  vwarn(const std::shared_ptr<logger>& logger, const std::string_view fmt,
        Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::warn)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(0, level::warn, buf, source);
    } catch (...) {
    }
  }

  vwarn(const std::shared_ptr<logger>& logger, const std::u8string_view fmt,
        Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::warn)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(0, level::warn, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
vwarn(const std::shared_ptr<logger>& logger, std::string_view fmt,
      Args&&... args) -> vwarn<Args...>;

template <typename... Args>
vwarn(const std::shared_ptr<logger>& logger, std::u8string_view fmt,
      Args&&... args) -> vwarn<Args...>;

template <typename... Args>
struct vinfo {
  vinfo(const std::shared_ptr<logger>& logger, const std::string_view fmt,
        Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::info)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(0, level::info, buf, source);
    } catch (...) {
    }
  }

  vinfo(const std::shared_ptr<logger>& logger, const std::u8string_view fmt,
        Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::info)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(0, level::info, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
vinfo(const std::shared_ptr<logger>& logger, std::string_view fmt,
      Args&&... args) -> vinfo<Args...>;

template <typename... Args>
vinfo(const std::shared_ptr<logger>& logger, std::u8string_view fmt,
      Args&&... args) -> vinfo<Args...>;

template <typename... Args>
struct vtrace {
  vtrace(const std::shared_ptr<logger>& logger, const std::string_view fmt,
         Args&&... args,
         const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::trace)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(0, level::trace, buf, source);
    } catch (...) {
    }
  }

  vtrace(const std::shared_ptr<logger>& logger, const std::u8string_view fmt,
         Args&&... args,
         const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::trace)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(0, level::trace, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
vtrace(const std::shared_ptr<logger>& logger, std::string_view fmt,
       Args&&... args) -> vtrace<Args...>;

template <typename... Args>
vtrace(const std::shared_ptr<logger>& logger, std::u8string_view fmt,
       Args&&... args) -> vtrace<Args...>;

}  // namespace jt::log