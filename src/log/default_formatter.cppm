// 默认日志格式化器实现
// 提供默认的日志格式化功能，包括时间戳、日志级别、线程ID等

module jt:log.default_formatter;

import std;
import :detail.buffer;
import :log.level;
import :log.formatter;
import :log.record;

namespace jt::log {

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
  void format(const log_record_view& record, detail::buffer_1k& output,
              std::size_t& color_start, std::size_t& color_stop) override {
    using namespace std::chrono;
    if (const auto current_second = system_clock::to_time_t(record.timestamp);
        current_second != last_second_) {
      last_second_ = current_second;
      date_and_time_.clear();
      const auto local_time = std::chrono::zoned_time{
          std::chrono::current_zone(),
          std::chrono::floor<std::chrono::seconds>(record.timestamp)};
      std::format_to(std::back_inserter(date_and_time_), "{:%Y-%m-%d %H:%M:%S}",
                     local_time);
      zone_offset_.clear();
      std::format_to(std::back_inserter(zone_offset_), "{:%Ez}", local_time);
    }

    const auto millis = time_fraction<milliseconds>(record.timestamp);
    std::string_view time_view(date_and_time_);
    std::string_view zone_offset_view(zone_offset_);
    std::format_to(std::back_inserter(output), "[{}.{:03} {}] [", time_view,
                   millis.count(), zone_offset_view);
    color_start = output.readable();
    output.append(to_string_view(record.lv));
    color_stop = output.readable();
    std::format_to(std::back_inserter(output), "] [{:5}] ", record.thread_id);
    if (record.service_id > 0) {
      std::format_to(std::back_inserter(output), "[{:5}] ", record.service_id);
    }
    std::string_view file_name = record.source.file_name();
    if (const auto pos = file_name.find_last_of("/\\");
        pos != std::string_view::npos) {
      file_name = file_name.substr(pos + 1);
    }
    std::format_to(std::back_inserter(output), "[{}:{}] ", file_name,
                   record.source.line());
    output.append(record.payload);
    output.append("\n", 1);
  }

 private:
  detail::base_memory_buffer<128> date_and_time_{};
  detail::base_memory_buffer<32> zone_offset_{};
  std::time_t last_second_{0};
};

}  // namespace jt::log
