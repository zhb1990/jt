/**
 * 日志工具类模块
 * 提供各类日志级别的日志记录功能
 * 通过类的构造函数的std::source_location::current()获取当前代码路径和行号
 * 使用方式: 在代码中创建临时对象即可自动记录日志
 * @note 此类为模板类，构造时即完成日志记录，无需显式调用
 * @note 支持两种形式的日志: 格式化日志(log, critical, error, warn, info, debug,
 * trace)和可变参数日志(vlog, vcritical, verror, vwarn, vinfo, vtrace)
 */

export module jt:log.functions;

import :log.logger;

export namespace jt::log {

/**
 * 通用日志记录类
 * @tparam Args 日志参数类型
 * @note 通过构造函数获取源代码位置信息，自动记录日志
 */
template <typename... Args>
struct log {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param lv 日志级别
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param lv 日志级别
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

/**
 * 类型推导指引 - log（不带服务ID）
 */
template <typename... Args>
log(const std::shared_ptr<logger>& logger, level lv,
    std::format_string<Args...> fmt, Args&&... args) -> log<Args...>;

/**
 * 类型推导指引 - log（带服务ID）
 */
template <typename... Args>
log(std::uint32_t sid, const std::shared_ptr<logger>& logger, level lv,
    std::format_string<Args...> fmt, Args&&... args) -> log<Args...>;

/**
 * 严重错误级别日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::critical，用于记录严重错误
 */
template <typename... Args>
struct critical {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

/**
 * 类型推导指引 - critical（不带服务ID）
 */
template <typename... Args>
critical(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
         Args&&... args) -> critical<Args...>;

/**
 * 类型推导指引 - critical（带服务ID）
 */
template <typename... Args>
critical(std::uint32_t sid, const std::shared_ptr<logger>& logger,
         std::format_string<Args...> fmt, Args&&... args) -> critical<Args...>;

/**
 * 错误级别日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::error，用于记录错误
 */
template <typename... Args>
struct error {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

/**
 * 类型推导指引 - error（不带服务ID）
 */
template <typename... Args>
error(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> error<Args...>;

/**
 * 类型推导指引 - error（带服务ID）
 */
template <typename... Args>
error(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::format_string<Args...> fmt, Args&&... args) -> error<Args...>;

/**
 * 警告级别日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::warn，用于记录警告信息
 */
template <typename... Args>
struct warn {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

/**
 * 类型推导指引 - warn（不带服务ID）
 */
template <typename... Args>
warn(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
     Args&&... args) -> warn<Args...>;

/**
 * 类型推导指引 - warn（带服务ID）
 */
template <typename... Args>
warn(std::uint32_t sid, const std::shared_ptr<logger>& logger,
     std::format_string<Args...> fmt, Args&&... args) -> warn<Args...>;

/**
 * 信息级别日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::info，用于记录一般信息
 */
template <typename... Args>
struct info {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

/**
 * 类型推导指引 - info（不带服务ID）
 */
template <typename... Args>
info(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
     Args&&... args) -> info<Args...>;

/**
 * 类型推导指引 - info（带服务ID）
 */
template <typename... Args>
info(std::uint32_t sid, const std::shared_ptr<logger>& logger,
     std::format_string<Args...> fmt, Args&&... args) -> info<Args...>;

/**
 * 调试级别日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::debug，用于记录调试信息
 */
template <typename... Args>
struct debug {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

/**
 * 类型推导指引 - debug（不带服务ID）
 */
template <typename... Args>
debug(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> debug<Args...>;

/**
 * 类型推导指引 - debug（带服务ID）
 */
template <typename... Args>
debug(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::format_string<Args...> fmt, Args&&... args) -> debug<Args...>;

/**
 * 跟踪级别日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::trace，用于记录跟踪信息
 */
template <typename... Args>
struct trace {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式化字符串
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

/**
 * 类型推导指引 - trace（不带服务ID）
 */
template <typename... Args>
trace(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> trace<Args...>;

/**
 * 类型推导指引 - trace（带服务ID）
 */
template <typename... Args>
trace(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::format_string<Args...> fmt, Args&&... args) -> trace<Args...>;

/**
 * 可变参数通用日志类
 * @tparam Args 日志参数类型
 * @note 使用std::string_view作为格式字符串，适用于运行时确定的格式
 */
template <typename... Args>
struct vlog {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param lv 日志级别
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param lv 日志级别
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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
};

/**
 * 类型推导指引 - vlog（不带服务ID）
 */
template <typename... Args>
vlog(const std::shared_ptr<logger>& logger, level lv, std::string_view fmt,
     Args&&... args) -> vlog<Args...>;

/**
 * 类型推导指引 - vlog（带服务ID）
 */
template <typename... Args>
vlog(std::uint32_t sid, const std::shared_ptr<logger>& logger, level lv,
     std::string_view fmt, Args&&... args) -> vlog<Args...>;

/**
 * 可变参数严重错误日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::critical，使用string_view作为格式字符串
 */
template <typename... Args>
struct vcritical {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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
};

/**
 * 类型推导指引 - vcritical（不带服务ID）
 */
template <typename... Args>
vcritical(const std::shared_ptr<logger>& logger, std::string_view fmt,
          Args&&... args) -> vcritical<Args...>;

/**
 * 类型推导指引 - vcritical（带服务ID）
 */
template <typename... Args>
vcritical(std::uint32_t sid, const std::shared_ptr<logger>& logger,
          std::string_view fmt, Args&&... args) -> vcritical<Args...>;

/**
 * 可变参数错误日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::error，使用string_view作为格式字符串
 */
template <typename... Args>
struct verror {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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
};

/**
 * 类型推导指引 - verror（不带服务ID）
 */
template <typename... Args>
verror(const std::shared_ptr<logger>& logger, std::string_view fmt,
       Args&&... args) -> verror<Args...>;

/**
 * 类型推导指引 - verror（带服务ID）
 */
template <typename... Args>
verror(std::uint32_t sid, const std::shared_ptr<logger>& logger,
       std::string_view fmt, Args&&... args) -> verror<Args...>;

/**
 * 可变参数警告日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::warn，使用string_view作为格式字符串
 */
template <typename... Args>
struct vwarn {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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
};

/**
 * 类型推导指引 - vwarn（不带服务ID）
 */
template <typename... Args>
vwarn(const std::shared_ptr<logger>& logger, std::string_view fmt,
      Args&&... args) -> vwarn<Args...>;

/**
 * 类型推导指引 - vwarn（带服务ID）
 */
template <typename... Args>
vwarn(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::string_view fmt, Args&&... args) -> vwarn<Args...>;

/**
 * 可变参数信息日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::info，使用string_view作为格式字符串
 */
template <typename... Args>
struct vinfo {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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
};

/**
 * 类型推导指引 - vinfo（不带服务ID）
 */
template <typename... Args>
vinfo(const std::shared_ptr<logger>& logger, std::string_view fmt,
      Args&&... args) -> vinfo<Args...>;

/**
 * 类型推导指引 - vinfo（带服务ID）
 */
template <typename... Args>
vinfo(std::uint32_t sid, const std::shared_ptr<logger>& logger,
      std::string_view fmt, Args&&... args) -> vinfo<Args...>;

/**
 * 可变参数跟踪日志
 * @tparam Args 日志参数类型
 * @note 日志级别为level::trace，使用string_view作为格式字符串
 */
template <typename... Args>
struct vtrace {
  /**
   * 构造函数（不带服务ID）
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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

  /**
   * 构造函数（带服务ID）
   * @param sid 服务ID
   * @param logger 日志记录器共享指针
   * @param fmt 格式字符串（string_view）
   * @param args 日志参数
   * @param source 源代码位置（自动获取）
   */
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
};

/**
 * 类型推导指引 - vtrace（不带服务ID）
 */
template <typename... Args>
vtrace(const std::shared_ptr<logger>& logger, std::string_view fmt,
       Args&&... args) -> vtrace<Args...>;

/**
 * 类型推导指引 - vtrace（带服务ID）
 */
template <typename... Args>
vtrace(std::uint32_t sid, const std::shared_ptr<logger>& logger,
       std::string_view fmt, Args&&... args) -> vtrace<Args...>;

}  // namespace jt::log
