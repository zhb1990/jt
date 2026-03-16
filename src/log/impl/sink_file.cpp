module;

#include <rapidjson/document.h>

// module jt:log.sink.file;
module jt;

import std;

namespace jt::log {

/**
 * 文件日志Sink的实现类
 * 负责日志的写入、文件轮换、日志压缩等功能
 */
class sink_file_imp {
 public:
  /**
   * 构造函数
   * @param service 日志服务引用
   * @param config 文件日志配置
   */
  explicit sink_file_imp(service& s, const sink_file_config& config)  // NOLINT
      : service_(s),
        max_size_(config.max_size),
        daily_rotation_(config.daily_rotation),
        keep_days_(config.keep_days) {
    // 保存配置信息
    name_ = config.name;
    directory_ = config.directory;
    lz4_directory_ = config.lz4_directory;

    // 创建清单文件路径（manifest文件用于存储日志轮换状态）
    detail::buffer_1k temp;
    std::format_to(std::back_inserter(temp), "manifest_{}.json", name_);

    std::u8string_view u8strv(
        reinterpret_cast<const char8_t*>(directory_.c_str()),
        directory_.size());
    manifest_path_ = u8strv;
    std::error_code ec;
    create_directories(manifest_path_, ec);
    u8strv = {reinterpret_cast<const char8_t*>(temp.begin_read()),
              temp.readable()};
    manifest_path_ /= u8strv;
    load_manifest();  // 加载之前的清单信息

    // 创建LZ4压缩目录
    u8strv = {reinterpret_cast<const char8_t*>(lz4_directory_.c_str()),
              lz4_directory_.size()};
    const std::filesystem::path lz4_directory = u8strv;
    create_directories(lz4_directory, ec);
  }

  /**
   * 写入日志数据到文件
   * @param point 时间点
   * @param buf 日志缓冲区
   */
  void write(const sink::time_point& point, const detail::buffer_1k& buf) {
    // 检查是否需要进行日志轮换（基于时间或文件大小）
    if (tomorrow_ < point) {
      const auto old_day = manifest_.day;
      tomorrow_ =
          std::chrono::floor<std::chrono::days>(point) + std::chrono::days(1);
      if (daily_rotation_ || manifest_.day == 0) {
        rotate();  // 执行日志轮换
      }

      // 清理过期的压缩日志文件
      if (keep_days_ > 0 && old_day > 0) {
        service_.clear_lz4(name_, lz4_directory_, keep_days_);
      }
    }

    // 检查文件大小是否超过限制
    if (file_size_ >= max_size_) {
      rotate();  // 执行日志轮换
    }

    // 确保文件已打开
    if (!file_.is_open()) {
      file_open();
      if (!file_.is_open()) {
        return;
      }
    }

    // 写入日志数据并更新文件大小计数
    file_.write(reinterpret_cast<const char*>(buf.begin_read()),
                static_cast<std::streamsize>(buf.readable()));
    file_size_ += buf.readable();
  }

  /**
   * 刷新文件缓冲区
   * 确保所有缓冲的数据都写入磁盘
   */
  void flush_unlock() {  // NOLINT(*-convert-member-functions-to-static)
    if (file_.is_open()) {
      file_.flush();
    }
  }

 private:
  /**
   * 加载清单文件（manifest）
   * 读取之前保存的日志轮换状态（日期和序号）
   */
  void load_manifest() {
    std::ifstream manifest(manifest_path_, std::ios_base::binary);
    if (!manifest.is_open()) {
      return;
    }

    // 读取清单文件内容
    const detail::string data((std::istreambuf_iterator(manifest)),
                              std::istreambuf_iterator<char>());
    rapidjson::Document document;
    document.Parse(data.c_str(), data.size());
    if (document.HasParseError() || !document.IsObject()) {
      return;
    }

    const auto root = document.GetObject();
    auto it = root.FindMember("day");
    if (it == root.MemberEnd() || !it->value.IsUint()) {
      return;
    }

    // 获取日期和序号信息
    auto day = static_cast<std::uint32_t>(it->value.GetUint());
    it = root.FindMember("seq");
    if (it == root.MemberEnd() || !it->value.IsUint()) {
      return;
    }

    manifest_.seq = static_cast<std::uint32_t>(it->value.GetUint());
    manifest_.day = day;

    // 计算明天的日期用于轮换判断
    const int year = static_cast<int>(day / 10000);
    const int month = static_cast<int>(day % 10000) / 100;
    const int m_day = static_cast<int>(day % 100);
    tomorrow_ = std::chrono::year_month_day(std::chrono::year(year),
                                            std::chrono::month(month),
                                            std::chrono::day(m_day));
    tomorrow_ += std::chrono::days{1};

    // 确保文件已打开以便追加写入
    return file_open();
  }

  /**
   * 保存清单文件（manifest）
   * 将当前的日志轮换状态（日期和序号）写入磁盘
   */
  void save_manifest() {
    std::ofstream file(manifest_path_, std::ios::binary);
    detail::buffer_1k temp;
    std::format_to(std::back_inserter(temp), R"({{ "day":{}, "seq":{} }})",
                   manifest_.day, manifest_.seq);
    file.write(reinterpret_cast<const char*>(temp.begin_read()),
               static_cast<std::streamsize>(temp.readable()));
  }

  /**
   * 执行日志轮换操作
   * 关闭当前日志文件，触发LZ4压缩，并创建新的日志文件
   */
  void rotate() {
    // 关闭当前文件并重置大小计数
    if (file_.is_open()) {
      file_.close();
      file_size_ = 0;
      // 触发当前日志文件的LZ4压缩
      service_.post_lz4(file_name_, lz4_directory_);
    }

    // 计算今天的日期（用于命名新日志文件）
    const std::chrono::year_month_day today{tomorrow_ - std::chrono::days{1}};
    const std::int32_t day = int{today.year()} * 10000 +
                             unsigned{today.month()} * 100 +
                             unsigned{today.day()};

    // 如果是新的一天，重置序号；否则增加序号
    if (manifest_.day < day) {  // NOLINT(*-branch-clone)
      manifest_.day = day;
      manifest_.seq = 0;
      save_manifest();
    } else {
      ++manifest_.seq;
      save_manifest();
    }
  }

  /**
   * 打开日志文件进行追加写入
   * 根据当前的日期和序号生成文件名
   */
  void file_open() {
    detail::buffer_1k temp;
    // 生成日志文件名：{name}_{date}.log 或 {name}_{date}_{seq:04d}.log
    if (manifest_.seq == 0) {  // NOLINT(*-branch-clone)
      std::format_to(std::back_inserter(temp), "{}_{}.log", name_,
                     manifest_.day);
    } else {
      std::format_to(std::back_inserter(temp), "{}_{}_{:04d}.log", name_,
                     manifest_.day, manifest_.seq);
    }

    // 构建完整的文件路径
    std::u8string_view u8strv(
        reinterpret_cast<const char8_t*>(directory_.c_str()),
        directory_.size());
    file_name_ = u8strv;
    u8strv = {reinterpret_cast<const char8_t*>(temp.begin_read()),
              temp.readable()};
    file_name_ /= u8strv;

    // 如果文件已存在，获取其大小用于续写
    if (std::filesystem::exists(file_name_)) {
      file_size_ = std::filesystem::file_size(file_name_);
    }
    // 以追加模式打开文件
    file_.open(file_name_, std::ios::binary | std::ios::app);
  }

  // 服务引用
  service& service_;
  // 日志文件基础名称
  detail::string name_{};
  // 日志文件目录
  detail::string directory_{};
  // LZ4压缩文件目录
  detail::string lz4_directory_{};
  // 单个日志文件最大大小
  std::size_t max_size_;
  // 是否启用每日轮换
  bool daily_rotation_;
  // 保留压缩日志文件的天数
  std::uint32_t keep_days_;

  /**
   * 清单文件结构
   * 用于保存日志轮换的状态信息
   */
  struct manifest {
    // 年月日，例如20260102（格式：YYYYMMDD）
    std::uint32_t day{0};
    // 日志序号（同一天内的文件序号）
    std::uint32_t seq{0};
  };
  manifest manifest_{};                  // 当前的清单状态
  std::filesystem::path manifest_path_;  // 清单文件路径
  std::ofstream file_;                   // 日志文件流
  std::filesystem::path file_name_;      // 当前日志文件路径
  std::size_t file_size_{0};             // 当前日志文件大小
  std::chrono::sys_days tomorrow_{};     // 明天的日期（用于判断是否需要轮换）
};

/**
 * 文件日志Sink的包装类
 * 通过PIMPL idiom隐藏实现细节
 */
sink_file::sink_file(service& s, const sink_file_config& config)  // NOLINT
    : impl_(detail::make_unique<sink_file_imp>(s, config)) {}

sink_file::~sink_file() noexcept = default;

/**
 * 写入日志条目
 * @param level 日志级别
 * @param point 时间点
 * @param buf 日志缓冲区
 */
void sink_file::write(level, const time_point& point,
                      const detail::buffer_1k& buf, std::size_t, std::size_t) {
  return impl_->write(point, buf);
}

/**
 * 刷新日志文件
 * 确保所有缓冲的数据都写入磁盘
 */
void sink_file::flush_unlock() { return impl_->flush_unlock(); }

}  // namespace jt::log