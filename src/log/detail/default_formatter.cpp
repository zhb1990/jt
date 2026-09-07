module jt.log.default_formatter;

import std;
import jt.base.buffer;
import jt.log.level;
import jt.log.record;
import jt.log.format;

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

void default_formatter::format(const log_record_view& record,
                               base::buffer_1k& output,
                               std::size_t& color_start,
                               std::size_t& color_stop) {
  using namespace std::chrono;
  if (const auto current_second = system_clock::to_time_t(record.timestamp);
      current_second != last_second_) {
    last_second_ = current_second;
    format_local_time(record.timestamp, date_and_time_, zone_offset_);
  }

  const auto millis = time_fraction<milliseconds>(record.timestamp);
  std::string_view time_view(date_and_time_);
  std::string_view zone_offset_view(zone_offset_);
  jt::log::format_to(output, "[{}.{:03} {}] [", time_view, millis.count(),
                     zone_offset_view);
  color_start = output.readable();
  output.append(to_string_view(record.lv));
  color_stop = output.readable();
  jt::log::format_to(output, "] [{:5}] ", record.thread_id);
  if (record.service_id > 0) {
    jt::log::format_to(output, "[{:5}] ", record.service_id);
  }
  std::string_view file_name = record.source.file_name();
  if (const auto pos = file_name.find_last_of("/\\");
      pos != std::string_view::npos) {
    file_name = file_name.substr(pos + 1);
  }
  jt::log::format_to(output, "[{}:{}] ", file_name, record.source.line());
  output.append(record.payload);
  output.append("\n", 1);
}

}  // namespace jt::log
