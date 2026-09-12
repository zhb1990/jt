module;

#include "../../detail/platform/config.h"

export module jt.log.core:service;

import std;
import jt.base.memory;
import jt.base.buffer;
import jt.base.containers;
import jt.log.level;
import jt.log.sink;
import :fwd;

namespace jt::log {
class service_impl;
}

export namespace jt::log {

/**
 * 日志服务类
 * 提供日志记录器的创建、查找和管理功能，以及日志的异步处理
 */
class service {
 public:
  /** 日志记录器共享指针类型别名 */
  using logger_sptr = std::shared_ptr<logger>;
  /** 日志记录器弱指针类型别名（避免循环引用） */
  using logger_wptr = std::weak_ptr<logger>;
  /** Sink智能指针类型别名 */
  using sink_ptr = base::dynamic_unique_ptr<sink>;

  /**
   * 构造函数
   * 创建一个日志服务实例
   */
  JT_API service();

  /**
   * 析构函数
   */
  JT_API ~service() noexcept;

  service(const service&) = delete;
  service(service&&) = delete;
  service& operator=(const service&) = delete;
  service& operator=(service&&) = delete;

  /**
   * 查找日志记录器
   * @param name 日志记录器名称
   * @return 如果找到返回共享指针，否则返回空指针
   */
  JT_API logger_sptr find(std::string_view name);

  /**
   * 删除日志记录器
   * @param name 要删除的日志记录器名称
   */
  JT_API void erase(std::string_view name);

  /**
   * 清除所有日志记录器
   */
  JT_API void clear();

  /**
   * 请求停止日志服务
   * 停止后台写入和压缩线程
   */
  JT_API void request_stop();

  /**
   * 获取默认日志记录器
   * @return 默认日志记录器的共享指针
   */
  JT_API auto get_default() -> logger_sptr;

  /**
   * 设置默认日志记录器
   * @param ptr 要设置为默认的日志记录器
   */
  JT_API void set_default(const logger_sptr& ptr);

  /**
   * 刷新日志记录器
   * @param ptr 要刷新的日志记录器弱指针
   */
  JT_API void flush(const logger_wptr& ptr);

  /**
   * 记录日志
   * @param ptr 日志记录器弱指针
   * @param sid 服务ID
   * @param lv 日志级别
   * @param buf 日志内容缓冲区
   * @param source 源代码位置信息
   */
  JT_API void log(const logger_wptr& ptr, std::uint32_t sid, level lv,
                  base::buffer_1k& buf, const std::source_location& source);

  /**
   * 创建日志记录器（范围版本）
   * @param range Sink范围
   * @param name 日志记录器名称
   * @param async 是否异步模式
   * @return 创建的日志记录器
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, sink_ptr>
  auto create_logger(R&& range, const std::string_view& name, const bool async)
      -> logger_sptr {
    base::vector<sink_ptr> sinks;
    auto iter = std::ranges::begin(range);
    const auto end = std::ranges::end(range);
    for (; iter != end; ++iter) {
      sinks.push_back(std::ranges::iter_move(iter));
    }
    return create_logger(name, async, std::move(sinks));
  }

  /**
   * 创建日志记录器（向量版本）
   * @param name 日志记录器名称
   * @param async 是否异步模式
   * @param sinks Sink集合
   * @return 创建的日志记录器
   */
  JT_API auto create_logger(const std::string_view& name, bool async,
                            base::vector<sink_ptr> sinks) -> logger_sptr;

  /**
   * 给文件 Sink 用的弱引用句柄：service 销毁后调用会自行失效。
   */
  class lz4_client {
   public:
    JT_API void post(const std::filesystem::path& file_name,
                     std::string_view lz4_directory) const;
    JT_API void clear(std::string_view name, std::string_view lz4_directory,
                      std::uint32_t keep_days) const;

   private:
    friend class service;
    explicit lz4_client(std::weak_ptr<void> impl) : impl_(std::move(impl)) {}
    std::weak_ptr<void> impl_;
  };

  [[nodiscard]] JT_API auto make_lz4_client() const -> lz4_client;

 private:
  friend class logger;

  auto get_impl() -> std::shared_ptr<service_impl>;

  /** Pimpl idiom实现指针 */
  std::shared_ptr<service_impl> impl_;
};

}  // namespace jt::log
