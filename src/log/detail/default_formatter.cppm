// 默认日志格式化器实现
// 提供默认的日志格式化功能，包括时间戳、日志级别、线程ID等

export module jt.log.default_formatter;

import std;
import jt.base.buffer;
import jt.log.level;
import jt.log.formatter;
import jt.log.record;

export namespace jt::log {

/**
 * 默认日志格式化器
 * 实现formatter接口，提供标准的日志格式化输出
 * 格式: [时间.毫秒] [级别] [{线程ID}] [{服务ID}] [文件:行号] 内容
 */
class default_formatter final : public formatter {
 public:
  void format(const log_record_view& record, base::buffer_1k& output,
              std::size_t& color_start, std::size_t& color_stop) override;

 private:
  base::base_memory_buffer<128> date_and_time_{};
  base::base_memory_buffer<32> zone_offset_{};
  std::time_t last_second_{0};
};

}  // namespace jt::log
