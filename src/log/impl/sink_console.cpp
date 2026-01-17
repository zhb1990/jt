module;

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>

#include <cstdio>
#endif

// module jt:log.sink.console;
module jt;

import std;

namespace jt::log {

#ifndef _WIN32

constexpr std::string_view color_reset = "\033[m";
constexpr std::string_view color_white = "\033[37m";
constexpr std::string_view color_green = "\033[32m";
constexpr std::string_view color_cyan = "\033[36m";
constexpr std::string_view color_yellow_bold = "\033[33m\033[1m";
constexpr std::string_view color_red_bold = "\033[31m\033[1m";
constexpr std::string_view color_bold_on_red = "\033[1m\033[41m";

// Determine if the terminal supports colors
// Based on: https://github.com/agauniyal/rang/
bool is_color_terminal() noexcept {
  static const bool result = []() {
    const char* env_colorterm_p = std::getenv("COLORTERM");
    if (env_colorterm_p != nullptr) {
      return true;
    }

    constexpr std::array terms{"ansi",  "color",   "console",   "cygwin",
                               "gnome", "konsole", "kterm",     "linux",
                               "msys",  "putty",   "rxvt",      "screen",
                               "vt100", "xterm",   "alacritty", "vt102"};
    const char* env_term_p = std::getenv("TERM");
    if (env_term_p == nullptr) {
      return false;
    }

    return std::any_of(terms.begin(), terms.end(), [&](const char* term) {
      return std::strstr(env_term_p, term) != nullptr;
    });
  }();

  return result;
}

// Determine if the terminal attached
// Source: https://github.com/agauniyal/rang/
bool in_terminal(std::FILE* file) { return ::isatty(::fileno(file)) != 0; }

#endif

class sink_console_impl {
 public:
#ifdef _WIN32
  explicit sink_console_impl(const DWORD handle) {
    handle_ = GetStdHandle(handle);
    DWORD mode;
    enable_color_ = GetConsoleMode(handle_, &mode) != 0;
  }
#else
  explicit sink_console_impl(std::FILE* handle) : handle_(handle) {
    enable_color_ = is_color_terminal() && in_terminal(handle_);
  }
#endif

  void write(const level lv, const sink::time_point&,
             const detail::buffer_1k& buf, const std::size_t color_start,
             const std::size_t color_stop) {
    std::lock_guard lock(mutex_);
    if (enable_color_ && color_stop > color_start) {
      write_range(buf, 0, color_start);
      set_color(lv);
      write_range(buf, color_start, color_stop);
      reset_color();
      write_range(buf, color_stop, buf.readable());
    } else {
      write_range(buf, 0, buf.readable());
    }
  }

  void flush_unlock() {
#ifndef _WIN32
    std::lock_guard lock(mutex_);
    std::fflush(handle_);
#endif
  }

 private:
  void write_range(const detail::buffer_1k& buf, const std::size_t start,
                   const std::size_t end) {
#ifdef _WIN32
    auto* ptr = reinterpret_cast<const char*>(buf.begin_read()) + start;
    const int str_len = static_cast<int>(end - start);
    const auto len = MultiByteToWideChar(CP_UTF8, 0, ptr, str_len, nullptr, 0);
    if (len <= 0) return;

    std::wstring temp(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, ptr, str_len, temp.data(),
                        static_cast<int>(temp.size()));
    DWORD written;
    WriteConsoleW(handle_, temp.c_str(), static_cast<DWORD>(temp.size()),
                  &written, nullptr);
#else
    std::fwrite(buf.begin_read() + start, sizeof(char), end - start, handle_);
#endif
  }

  void set_color(const level lv) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
    if (!::GetConsoleScreenBufferInfo(handle_, &orig_buffer_info)) {
      return;
    }

    if (old_attribs_ == 0) {
      old_attribs_ = orig_buffer_info.wAttributes;
    }

    WORD new_attribs = 0;
    switch (lv) {
      case level::critical:
        new_attribs = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN |
                      FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
      case level::error:
        new_attribs = FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
      case level::warn:
        new_attribs = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
      case level::info:
        new_attribs = FOREGROUND_GREEN;
        break;
      case level::debug:
        new_attribs = FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
      default:
        new_attribs = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
    }

    ::SetConsoleTextAttribute(
        handle_, new_attribs | (orig_buffer_info.wAttributes & 0xfff0));
#else
    switch (lv) {
      case level::critical:
        std::fwrite(color_bold_on_red.data(), sizeof(char),
                    color_bold_on_red.size(), handle_);
        return;
      case level::error:
        std::fwrite(color_red_bold.data(), sizeof(char), color_red_bold.size(),
                    handle_);
        return;
      case level::warn:
        std::fwrite(color_yellow_bold.data(), sizeof(char),
                    color_yellow_bold.size(), handle_);
        return;
      case level::info:
        std::fwrite(color_cyan.data(), sizeof(char), color_cyan.size(),
                    handle_);
        return;
      case level::debug:
        std::fwrite(color_green.data(), sizeof(char), color_green.size(),
                    handle_);
        return;
      default:
        std::fwrite(color_white.data(), sizeof(char), color_white.size(),
                    handle_);
        return;
    }
#endif
  }

  void reset_color() {
#ifdef _WIN32
    if (old_attribs_ > 0) {
      ::SetConsoleTextAttribute(handle_, old_attribs_);
      old_attribs_ = 0;
    }
#else
    std::fwrite(color_reset.data(), sizeof(char), color_reset.size(), handle_);
#endif
  }

  static std::mutex mutex_;
#ifdef _WIN32
  HANDLE handle_{INVALID_HANDLE_VALUE};
  WORD old_attribs_{0};
#else
  std::FILE* handle_{nullptr};
#endif
  bool enable_color_{false};
};

std::mutex sink_console_impl::mutex_;

#ifdef _WIN32
sink_console_impl console_stdout(STD_OUTPUT_HANDLE);
sink_console_impl console_stderr(STD_ERROR_HANDLE);
#else
sink_console_impl console_stdout(stdout);
sink_console_impl console_stderr(stderr);
#endif

sink_stdout::sink_stdout() : impl_(console_stdout) {}

sink_stdout::~sink_stdout() noexcept = default;

void sink_stdout::write(const level lv, const time_point& point,
                        const detail::buffer_1k& buf,
                        const std::size_t color_start,
                        const std::size_t color_stop) {
  return impl_.write(lv, point, buf, color_start, color_stop);
}

void sink_stdout::flush_unlock() { return impl_.flush_unlock(); }

sink_stderr::sink_stderr() : impl_(console_stderr) {}

sink_stderr::~sink_stderr() noexcept = default;

void sink_stderr::write(const level lv, const time_point& point,
                        const detail::buffer_1k& buf,
                        const std::size_t color_start,
                        const std::size_t color_stop) {
  return impl_.write(lv, point, buf, color_start, color_stop);
}

void sink_stderr::flush_unlock() { return impl_.flush_unlock(); }

}  // namespace jt::log