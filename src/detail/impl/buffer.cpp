module;

#include "../config.h"

// module jt:detail.buffer;
module jt;

namespace jt::detail {

/**
 * 收缩缓冲区，丢弃已读取的数据
 * 将已读取的数据移动到缓冲区开头，并重置读写指针
 */
void channel_buffer::shrink() noexcept {
  if (read_ == 0) return;

  const auto size = readable();
  std::memmove(data_, begin_read(), size);
  read_ = 0;
  write_ = size;
}

/**
 * 追加数据到缓冲区
 * @param buf 数据指针
 * @param len 数据长度
 */
void channel_buffer::append(const void* buf, std::size_t len) {
  len = (std::min)(len, writable());  // 确保不超过可写空间
  std::memmove(begin(), buf, len);    // 将数据移动到可写区域
  written(len);                       // 更新写入位置
}

/**
 * 追加只读缓冲区数据
 * @param buf 只读缓冲区
 * @note 此函数被标记为NOLINT以避免误报
 */
void channel_buffer::append(const read_buffer& buf) {  // NOLINT
  return append(buf.begin(), buf.readable());
}

/**
 * 追加字符串视图数据
 * @param strv 字符串视图
 * @note 此函数被标记为NOLINT以避免误报
 */
void channel_buffer::append(const std::string_view strv) {  // NOLINT
  return append(strv.data(), strv.size());
}

/**
 * 追加C字符串数据
 * @param str C字符串
 * @note 此函数被标记为NOLINT以避免误报
 */
void channel_buffer::append(const char* str) {  // NOLINT
  return append(str, std::strlen(str));
}

/**
 * 追加单个字节数据
 * @param val 要追加的字节值
 * @note 此函数被标记为NOLINT以避免误报
 */
void channel_buffer::push_back(const std::uint8_t val) {  // NOLINT
  return append(&val, sizeof(val));
}

/**
 * 查看数据（不移动读指针）
 * @param buf 目标缓冲区
 * @param sz 请求读取的字节数
 * @return 实际读取的字节数
 */
auto channel_buffer::peek(void* buf, std::size_t sz) const noexcept  // NOLINT
    -> std::size_t {
  sz = (std::min)(sz, readable());
  std::memmove(buf, begin_read(), sz);
  return sz;
}

/**
 * 反向查看数据（从末尾读取，不移动读指针）
 * @param buf 目标缓冲区
 * @param sz 请求读取的字节数
 * @return 实际读取的字节数
 */
auto channel_buffer::rpeek(void* buf, std::size_t sz) const noexcept  // NOLINT
    -> std::size_t {
  sz = (std::min)(sz, readable());
  std::memmove(buf, end_read() - sz, sz);
  return sz;
}

/**
 * 在前端 prepend 数据
 * @param buf 数据指针
 * @param len 数据长度
 * @return 是否成功（如果前置可写区域足够大则返回true，否则返回false）
 */
auto channel_buffer::prepend(const void* buf, const std::size_t len) noexcept
    -> bool {
  if (prependable() < len) return false;

  std::memmove(static_cast<std::uint8_t*>(data_) + read_ - len, buf, len);
  read_ -= len;
  return true;
}

// 显式实例化模板类以避免链接错误
template class JT_API base_memory_buffer<1024>;
template class JT_API base_memory_buffer<2048>;
template class JT_API base_memory_buffer<4096>;
template class JT_API base_memory_buffer<8192>;

}  // namespace jt::detail