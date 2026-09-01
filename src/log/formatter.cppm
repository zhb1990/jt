module;

#include "../detail/config.h"

export module jt:log.formatter;

import std;
import :detail.buffer;
import :log.record;

export namespace jt::log {

/**
 * 日志格式化器基类
 * 定义了格式化日志记录的纯虚函数接口
 * 具体的格式化器需要继承此类并实现format方法
 */
struct JT_API formatter {
  virtual ~formatter() noexcept;

  /**
   * 格式化日志记录
   * @param record 要格式化的只读日志记录
   * @param output 用于存储格式化结果的缓冲区
   * @param color_start 颜色开始位置（用于终端着色）
   * @param color_stop 颜色停止位置（用于终端着色）
   */
  virtual void format(const log_record_view& record, detail::buffer_1k& output,
                      std::size_t& color_start, std::size_t& color_stop) = 0;
};

}  // namespace jt::log
