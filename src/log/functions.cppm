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

namespace jt::log {

template <typename Format>
void write_log(std::uint32_t sid, const std::shared_ptr<logger>& logger_ptr,
               level lv, const std::source_location& source, Format&& format) {
  if (!logger_ptr->should_log(lv)) return;

  try {
    detail::buffer_1k buf;
    std::forward<Format>(format)(std::back_inserter(buf));
    logger_ptr->log(sid, lv, buf, source);
  } catch (...) {
  }
}

template <level Lv, typename... Args>
struct log {
  log(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args,
      const std::source_location& source = std::source_location::current()) {
    write_log(0, logger, Lv, source, [&](auto&& out) {
      std::format_to(out, fmt, std::forward<Args>(args)...);
    });
  }
};

template <level Lv, typename... Args>
struct vlog {
  vlog(const std::shared_ptr<logger>& logger, std::string_view fmt,
       Args&&... args,
       const std::source_location& source = std::source_location::current()) {
    write_log(0, logger, Lv, source, [&](auto&& out) {
      std::vformat_to(out, fmt, std::make_format_args(args...));
    });
  }
};

}  // namespace jt::log

export namespace jt::log {

template <typename... Args>
struct critical : log<level::critical, Args...> {
  using base = log<level::critical, Args...>;
  using base::base;
};

template <typename... Args>
struct vcritical : vlog<level::critical, Args...> {
  using base = vlog<level::critical, Args...>;
  using base::base;
};

template <typename... Args>
critical(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
         Args&&... args) -> critical<Args...>;

template <typename... Args>
vcritical(const std::shared_ptr<logger>& logger, std::string_view fmt,
          Args&&... args) -> vcritical<Args...>;

template <typename... Args>
struct error : log<level::error, Args...> {
  using base = log<level::error, Args...>;
  using base::base;
};

template <typename... Args>
struct verror : vlog<level::error, Args...> {
  using base = vlog<level::error, Args...>;
  using base::base;
};

template <typename... Args>
error(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> error<Args...>;

template <typename... Args>
verror(const std::shared_ptr<logger>& logger, std::string_view fmt,
       Args&&... args) -> verror<Args...>;

template <typename... Args>
struct warn : log<level::warn, Args...> {
  using base = log<level::warn, Args...>;
  using base::base;
};

template <typename... Args>
struct vwarn : vlog<level::warn, Args...> {
  using base = vlog<level::warn, Args...>;
  using base::base;
};

template <typename... Args>
warn(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
     Args&&... args) -> warn<Args...>;

template <typename... Args>
vwarn(const std::shared_ptr<logger>& logger, std::string_view fmt,
      Args&&... args) -> vwarn<Args...>;

template <typename... Args>
struct info : log<level::info, Args...> {
  using base = log<level::info, Args...>;
  using base::base;
};

template <typename... Args>
struct vinfo : vlog<level::info, Args...> {
  using base = vlog<level::info, Args...>;
  using base::base;
};

template <typename... Args>
info(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
     Args&&... args) -> info<Args...>;

template <typename... Args>
vinfo(const std::shared_ptr<logger>& logger, std::string_view fmt,
      Args&&... args) -> vinfo<Args...>;

template <typename... Args>
struct debug : log<level::debug, Args...> {
  using base = log<level::debug, Args...>;
  using base::base;
};

template <typename... Args>
struct vdebug : vlog<level::debug, Args...> {
  using base = vlog<level::debug, Args...>;
  using base::base;
};

template <typename... Args>
debug(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> debug<Args...>;

template <typename... Args>
vdebug(const std::shared_ptr<logger>& logger, std::string_view fmt,
       Args&&... args) -> vdebug<Args...>;

template <typename... Args>
struct trace : log<level::trace, Args...> {
  using base = log<level::trace, Args...>;
  using base::base;
};

template <typename... Args>
struct vtrace : vlog<level::trace, Args...> {
  using base = vlog<level::trace, Args...>;
  using base::base;
};

template <typename... Args>
trace(const std::shared_ptr<logger>& logger, std::format_string<Args...> fmt,
      Args&&... args) -> trace<Args...>;

template <typename... Args>
vtrace(const std::shared_ptr<logger>& logger, std::string_view fmt,
       Args&&... args) -> vtrace<Args...>;

}  // namespace jt::log
