// 日志格式化器基类接口
// 定义日志格式化器的抽象基类，所有具体的格式化器需要实现此接口

export module jt:log.formatter;

import std;
import :detail.buffer;
import :log.fwd;

export namespace jt::log {

/**
 * 日志格式化器基类
 * 定义了格式化日志消息的纯虚函数接口
 * 具体的格式化器需要继承此类并实现format方法
 */
struct formatter {
  virtual ~formatter() = default;

  /**
   * 格式化日志消息
   * @param msg 要格式化的日志消息
   * @param buf 用于存储格式化结果的缓冲区
   * @param color_start 颜色开始位置（用于终端着色）
   * @param color_stop 颜色结束位置（用于终端着色）
   * @note 此函数为纯虚函数，必须在派生类中实现
   */
  virtual void format(const message& msg, detail::buffer_1k& buf,
                      std::size_t& color_start, std::size_t& color_stop) = 0;
};

}  // namespace jt::log