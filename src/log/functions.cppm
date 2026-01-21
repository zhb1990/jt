export module jt:log.functions;

import :log.logger;

export namespace jt::log {

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

  log(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
      const level lv, std::format_string<Args...> fmt, Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(lv)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(sid, lv, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
log(const std::shared_ptr<logger>& logger, level lv,
    std::format_string<Args...> fmt, Args&&... args) -> log<Args...>;

template <typename... Args>
log(std::uint32_t sid, const std::shared_ptr<logger>& logger, level lv,
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

  critical(
      const std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::format_string<Args...> fmt, Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::critical)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(sid, level::critical, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
critical(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
         Args&&... args) -> critical<Args...>;

template <typename... Args>
critical(std::uint32_t sid, const std::shared_ptr<logger>& logger,
         std::format_string<Args...> fmt, Args&&... args) -> critical<Args...>;

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

  error(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
        std::format_string<Args...> fmt, Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::error)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(sid, level::error, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
error(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> error<Args...>;

template <typename... Args>
error(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::format_string<Args...> fmt, Args&&... args) -> error<Args...>;

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

  warn(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
       std::format_string<Args...> fmt, Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::warn)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(sid, level::warn, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
warn(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
     Args&&... args) -> warn<Args...>;

template <typename... Args>
warn(std::uint32_t sid, const std::shared_ptr<logger>& logger,
     std::format_string<Args...> fmt, Args&&... args) -> warn<Args...>;

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

  info(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
       std::format_string<Args...> fmt, Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::info)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(sid, level::info, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
info(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
     Args&&... args) -> info<Args...>;

template <typename... Args>
info(std::uint32_t sid, const std::shared_ptr<logger>& logger,
     std::format_string<Args...> fmt, Args&&... args) -> info<Args...>;

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

  debug(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
        std::format_string<Args...> fmt, Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::debug)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(sid, level::debug, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
debug(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> debug<Args...>;

template <typename... Args>
debug(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::format_string<Args...> fmt, Args&&... args) -> debug<Args...>;

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

  trace(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
        std::format_string<Args...> fmt, Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::trace)) return;

    try {
      detail::buffer_1k buf;
      std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
      logger->log(sid, level::trace, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
trace(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> trace<Args...>;

template <typename... Args>
trace(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::format_string<Args...> fmt, Args&&... args) -> trace<Args...>;

template <typename... Args>
struct vlog {
  vlog(const std::shared_ptr<logger>& logger, const level lv,
       const std::string_view fmt, Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(lv)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(0, lv, buf, source);
    } catch (...) {
    }
  }

  vlog(const std::shared_ptr<logger>& logger, const level lv,
       const std::u8string_view fmt, Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(lv)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(0, lv, buf, source);
    } catch (...) {
    }
  }

  vlog(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
       const level lv, const std::string_view fmt, Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(lv)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(sid, lv, buf, source);
    } catch (...) {
    }
  }

  vlog(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
       const level lv, const std::u8string_view fmt, Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(lv)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(sid, lv, buf, source);
    } catch (...) {
    }
  }
};

template <typename... Args>
vlog(const std::shared_ptr<logger>& logger, level lv, std::string_view fmt,
     Args&&... args) -> vlog<Args...>;

template <typename... Args>
vlog(const std::shared_ptr<logger>& logger, level lv, std::u8string_view fmt,
     Args&&... args) -> vlog<Args...>;

template <typename... Args>
vlog(std::uint32_t sid, const std::shared_ptr<logger>& logger, level lv,
     std::string_view fmt, Args&&... args) -> vlog<Args...>;

template <typename... Args>
vlog(std::uint32_t sid, const std::shared_ptr<logger>& logger, level lv,
     std::u8string_view fmt, Args&&... args) -> vlog<Args...>;

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

  vcritical(
      const std::uint32_t sid, const std::shared_ptr<logger>& logger,
      const std::string_view fmt, Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::critical)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(sid, level::critical, buf, source);
    } catch (...) {
    }
  }

  vcritical(
      const std::uint32_t sid, const std::shared_ptr<logger>& logger,
      const std::u8string_view fmt, Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::critical)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(sid, level::critical, buf, source);
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
vcritical(std::uint32_t sid, const std::shared_ptr<logger>& logger,
          std::string_view fmt, Args&&... args) -> vcritical<Args...>;

template <typename... Args>
vcritical(std::uint32_t sid, const std::shared_ptr<logger>& logger,
          std::u8string_view fmt, Args&&... args) -> vcritical<Args...>;

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

  verror(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
         const std::string_view fmt, Args&&... args,
         const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::error)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(sid, level::error, buf, source);
    } catch (...) {
    }
  }

  verror(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
         const std::u8string_view fmt, Args&&... args,
         const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::error)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(sid, level::error, buf, source);
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
verror(std::uint32_t sid, const std::shared_ptr<logger>& logger,
       std::string_view fmt, Args&&... args) -> verror<Args...>;

template <typename... Args>
verror(std::uint32_t sid, const std::shared_ptr<logger>& logger,
       std::u8string_view fmt, Args&&... args) -> verror<Args...>;

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

  vwarn(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
        const std::string_view fmt, Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::warn)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(sid, level::warn, buf, source);
    } catch (...) {
    }
  }

  vwarn(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
        const std::u8string_view fmt, Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::warn)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(sid, level::warn, buf, source);
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
vwarn(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::string_view fmt, Args&&... args) -> vwarn<Args...>;

template <typename... Args>
vwarn(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::u8string_view fmt, Args&&... args) -> vwarn<Args...>;

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

  vinfo(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
        const std::string_view fmt, Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::info)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(sid, level::info, buf, source);
    } catch (...) {
    }
  }

  vinfo(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
        const std::u8string_view fmt, Args&&... args,
        const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::info)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(sid, level::info, buf, source);
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
vinfo(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::string_view fmt, Args&&... args) -> vinfo<Args...>;

template <typename... Args>
vinfo(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::u8string_view fmt, Args&&... args) -> vinfo<Args...>;

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

  vtrace(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
         const std::string_view fmt, Args&&... args,
         const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::trace)) return;

    try {
      detail::buffer_1k buf;
      std::vformat_to(std::back_inserter(buf), fmt,
                      std::make_format_args(args...));
      logger->log(sid, level::trace, buf, source);
    } catch (...) {
    }
  }

  vtrace(const std::uint32_t sid, const std::shared_ptr<logger>& logger,
         const std::u8string_view fmt, Args&&... args,
         const std::source_location& source = std::source_location::current()) {
    if (!logger->should_log(level::trace)) return;

    try {
      detail::buffer_1k buf;
      const std::string_view temp(reinterpret_cast<const char*>(fmt.data()),
                                  fmt.size());
      std::vformat_to(std::back_inserter(buf), temp,
                      std::make_format_args(args...));
      logger->log(sid, level::trace, buf, source);
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

template <typename... Args>
vtrace(std::uint32_t sid, const std::shared_ptr<logger>& logger,
       std::string_view fmt, Args&&... args) -> vtrace<Args...>;

template <typename... Args>
vtrace(std::uint32_t sid, const std::shared_ptr<logger>& logger,
       std::u8string_view fmt, Args&&... args) -> vtrace<Args...>;

}  // namespace jt::log
