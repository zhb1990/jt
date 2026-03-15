module;
 
#include "../detail/config.h"
 
export module jt:log.sink.file;
 
import std;
import :log.sink;
import :detail.memory;
import :detail.string;
 
export namespace jt::log {
 
/**
 * 文件日志Sink配置结构体
 * 包含文件日志Sink所需的所有配置参数
 */
struct sink_file_config { // NOLINT(*-pro-type-member-init)
  // 日志文件的基础名字
  std::string_view name;
  // 日志文件的目录
  std::string_view directory;
  // lz4文件的目录
  std::string_view lz4_directory;
  // 日志文件最大大小（默认200MB）
  std::size_t max_size{200 * 1024 * 1024};
  // 是否每日轮换日志文件
  bool daily_rotation{true};
  // 保留文件的时间 单位天
  std::uint32_t keep_days{30};
};
 
class sink_file_imp;
 
/**
 * 文件日志Sink类
 * 实现将日志写入文件的功能，支持日志轮换和LZ4压缩
 */
class JT_API sink_file : public sink {
  public:
   /**
    * 构造函数
    * @param service 日志服务引用
    * @param config 文件日志配置
    */
   explicit sink_file(service& s, const sink_file_config& config);
 
   /**
    * 析构函数
    */
   ~sink_file() noexcept override;
 
   /**
    * 写入日志条目
    * @param level 日志级别
    * @param point 时间点
    * @param buf 日志缓冲区
    */
   void write(level, const time_point& point, const detail::buffer_1k& buf,
              std::size_t, std::size_t) override;
 
   /**
    * 刷新日志文件
    * 确保所有缓冲的数据都写入磁盘
    */
   void flush_unlock() override;
 
  private:
   /** Pimpl idiom实现指针 */
   detail::unique_ptr<sink_file_imp> impl_;
 };
 
}  // namespace jt::log