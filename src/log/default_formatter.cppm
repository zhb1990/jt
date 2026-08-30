// 默认日志格式化器实现
// 提供默认的日志格式化功能，包括时间戳、日志级别、线程ID等

export module jt:log.default_formatter;

import std;
import :log.formatter;
import :log.message;

export namespace jt::log {

/**
 * 计算时间小数部分
 * @tparam ToDuration 目标持续时间类型
 * @param tp 时间点
 * @return 时间的小数部分（秒以下的部分）
 */
template <typename ToDuration>
ToDuration time_fraction(const std::chrono::system_clock::time_point& tp) {
  using std::chrono::duration_cast;
  using std::chrono::seconds;
  const auto duration = tp.time_since_epoch();
  const auto secs = duration_cast<seconds>(duration);
  return duration_cast<ToDuration>(duration) - duration_cast<ToDuration>(secs);
}

/**
 * 默认日志格式化器
 * 实现formatter接口，提供标准的日志格式化输出
 * 格式: [时间.毫秒] [级别] [{线程ID}] [{服务ID}] [文件:行号] 内容
 */
class default_formatter final : public formatter {
 public:
  /**
   * 格式化日志消息
   * @param msg 要格式化的日志消息
   * @param buf 用于存储格式化结果的缓冲区
   * @param color_start 颜色开始位置（用于终端着色）
   * @param color_stop 颜色结束位置（用于终端着色）
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  void format(const message& msg, detail::buffer_1k& buf,  // NOLINT
              std::size_t& color_start, std::size_t& color_stop) override {
    // 时间
    using namespace std::chrono;
    if (const auto current_second = system_clock::to_time_t(msg.point);
        current_second != last_second_) {
      last_second_ = current_second;
      date_and_time_.clear();
      std::format_to(std::back_inserter(date_and_time_), "{:%Y-%m-%d %H:%M:%S}",
                     std::chrono::floor<std::chrono::seconds>(msg.point));
    }

    const auto millis = time_fraction<milliseconds>(msg.point);
    std::string_view time_view(date_and_time_);
    std::format_to(std::back_inserter(buf), "[{}.{:03}] [", time_view,
                   millis.count());
    // 日志等级
    color_start = buf.readable();
    buf.append(to_string_view(msg.lv));
    color_stop = buf.readable();
    // 线程id
    std::format_to(std::back_inserter(buf), "] [{:5}] ", msg.tid);
    // 服务id
    if (msg.sid > 0) {
      std::format_to(std::back_inserter(buf), "[{:5}] ", msg.sid);
    }
    // 代码文件、行数
    std::string_view file_name = msg.source.file_name();
    if (const auto pos = file_name.find_last_of("/\\");
        pos != std::string_view::npos) {
      file_name = file_name.substr(pos + 1);
    }
    std::format_to(std::back_inserter(buf), "[{}:{}] ", file_name,
                   msg.source.line());
    // 内容
    buf.append(msg.buf.begin_read(), msg.buf.readable());
    buf.append("\n", 1);
  }

 private:
  // 用于存储日期和时间的缓冲区
  detail::base_memory_buffer<128> date_and_time_{};
  // 上次处理的秒数，用于避免重复格式化日期时间
  std::time_t last_second_{0};
};

}  // namespace jt::log