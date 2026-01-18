module;

#include "../detail/config.h"

export module jt:log.sink.file;

import std;
import :log.sink;
import :detail.memory;
import :detail.string;

export namespace jt::log {

struct sink_file_config {
  // 日志文件的基础名字
  std::string_view name;
  // 日志文件的目录
  std::string_view directory;
  // lz4文件的目录
  std::string_view lz4_directory;
  // 日志文件最大大小
  std::size_t max_size{200 * 1024 * 1024};
  // 是否每日轮换日志文件
  bool daily_rotation{true};
  // 保留文件的时间 单位天
  std::uint32_t keep_days{30};
};

class sink_file_imp;

class JT_API sink_file : public sink {
 public:
  sink_file(const sink_file_config& config);

  ~sink_file() noexcept override;

  void write(level, const time_point& point, const detail::buffer_1k& buf,
             std::size_t, std::size_t) override;

  void flush_unlock() override;

 private:
  detail::unique_ptr<sink_file_imp> impl_;
};

}  // namespace jt::log