module;

#include <lz4frame.h>

module jt:log.service_impl;

import std;
import :detail.memory;
import :detail.buffer;
import :detail.vector;
import :detail.string;
import :detail.deque;
import :detail.unordered_map;
import :detail.intrusive_mpsc_queue;
import :log.level;
import :log.sink;
import :log.fwd;
import :log.message;

namespace jt::log {

struct lz4_data {
  lz4_data();

  ~lz4_data() noexcept;

  void compress(const detail::string& src, const detail::string& directory);

  bool compress_file(std::ofstream& output, std::uint64_t& count_out,
                     std::uint64_t& count_in, std::ifstream& input);

  detail::vector<char> input_chunk;
  detail::vector<char> output_buff;
  LZ4F_compressionContext_t ctx{nullptr};
};

class service_impl {
 public:
  using logger_sptr = std::shared_ptr<logger>;
  using logger_wptr = std::weak_ptr<logger>;

  service_impl();

  ~service_impl();

  /**
   * 注册日志记录器
   * @param ptr 要注册的日志记录器共享指针
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  void register_logger(logger_sptr& ptr);

  /**
   * 查找日志记录器
   * @param name 日志记录器名称
   * @return 日志记录器共享指针，如果未找到则返回空指针
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  logger_sptr find(const std::string_view name);

  /**
   * 删除日志记录器
   * @param name 要删除的日志记录器名称
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  void erase(const std::string_view name);

  /**
   * 清除所有日志记录器
   * @note 此函数被标记为NOLINT以避免误报
   */
  void clear();

  /**
   * 请求停止日志服务
   */
  void request_stop();

  /**
   * 获取默认日志记录器
   * @return 默认日志记录器共享指针
   * @note 此函数被标记为NOLINT以避免误报
   */
  auto get_default() -> logger_sptr;

  /**
   * 设置默认日志记录器
   * @param ptr 要设置为默认的日志记录器共享指针
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  void set_default(const logger_sptr& ptr);

  /**
   * 刷新日志记录器
   * @param ptr 要刷新的日志记录器弱指针
   */
  void flush(const logger_wptr& ptr);

  /**
   * 记录日志消息
   * @param ptr 日志记录器弱指针
   * @param sid 来源ID
   * @param lv 日志级别
   * @param buf 日志缓冲区
   * @param source 源代码位置信息
   */
  void log(const logger_wptr& ptr, const std::uint32_t sid, const level lv,
           detail::buffer_1k& buf, const std::source_location& source);

  /**
   * 发送LZ4压缩请求
   * @param file_name 要压缩的文件名
   * @param lz4_directory LZ4目录
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  void post_lz4(const std::filesystem::path& file_name,  // NOLINT
                const std::string_view lz4_directory);

  /**
   * 清除LZ4压缩文件
   * @param name 文件名
   * @param lz4_directory LZ4目录
   * @param keep_days 保留天数
   */
  void clear_lz4(const detail::string& name,
                 const std::string_view lz4_directory,
                 const std::uint32_t keep_days);

 private:
  struct lz4_message;

  /**
   * 推送LZ4消息到队列
   * @param msg 要推送的LZ4消息
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  void push_lz4_message(lz4_message& msg);

  /**
   * 创建新的日志消息
   * @return 新的日志消息指针
   */
  message* new_log_message();

  /**
   * 删除日志消息
   * @param msg 要删除的日志消息指针
   */
  void delete_log_message(message* msg);

  /**
   * 推送日志消息到队列
   * @param msg 要推送的日志消息指针
   */
  void push_log_message(message* msg);

  /**
   * 处理日志消息
   */
  void writer_do_message();

  /**
   * 日志写入器线程运行函数
   */
  void writer_run();

  /**
   * 清除LZ4文件
   * @param msg 包含清除信息的LZ4消息
   * @note 此函数参数被标记为NOLINT以避免误报
   */
  void clear_lz4_files(const lz4_message& msg);

  /**
   * LZ4线程运行函数
   * @note 此函数被标记为NOLINT以避免误报
   */
  void lz4_run();

  struct lz4_message {  // NOLINT(*-pro-type-member-init)
    enum class type { lz4, clear };
    type tp{type::lz4};
    detail::string lz4_directory;
    detail::string file_name;
    std::uint32_t keep_days{0};
  };

  std::mutex loggers_mutex_{};
  detail::unordered_map<std::string_view, logger_sptr> loggers_{};
  std::atomic<logger_sptr> default_logger_{};

  std::thread writer_thread_{};
  std::mutex writer_mutex_{};
  std::condition_variable writer_cv_{};
  detail::intrusive_mpsc_queue<&message::next> writer_queue_{};

  std::thread lz4_thread_{};
  detail::deque<lz4_message> lz4_queue_{};
  std::mutex lz4_mutex_{};
  std::condition_variable_any lz4_cv_{};
  lz4_data lz4_data_;

  bool lz4_stop_requested_{false};
  bool writer_ready_{false};
  std::atomic_bool writer_stop_requested_{false};
  std::atomic<std::ptrdiff_t> writer_submission_counter_{0};
  detail::allocator<message> message_allocator_{};
};

}  // namespace jt::log