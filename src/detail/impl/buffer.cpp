module;

#include "../config.h"

// module jt:detail.buffer;
module jt;

namespace jt::detail {

void channel_buffer::shrink() noexcept {
  if (read_ == 0) return;

  const auto size = readable();
  std::memmove(data_, begin_read(), size);
  read_ = 0;
  write_ = size;
}

void channel_buffer::append(const void* buf, std::size_t len) {
  len = (std::min)(len, writable());
  std::memmove(begin(), buf, len);
  written(len);
}

void channel_buffer::append(const read_buffer& buf) {  // NOLINT
  return append(buf.begin(), buf.readable());
}

void channel_buffer::append(const std::string_view strv) {  // NOLINT
  return append(strv.data(), strv.size());
}

void channel_buffer::append(const char* str) {  // NOLINT
  return append(str, std::strlen(str));
}

void channel_buffer::push_back(const std::uint8_t val) {  // NOLINT
  return append(&val, sizeof(val));
}

auto channel_buffer::peek(void* buf, std::size_t sz) const noexcept  // NOLINT
    -> std::size_t {
  sz = (std::min)(sz, readable());
  std::memmove(buf, begin_read(), sz);
  return sz;
}

auto channel_buffer::rpeek(void* buf, std::size_t sz) const noexcept // NOLINT
    -> std::size_t {
  sz = (std::min)(sz, readable());
  std::memmove(buf, end_read() - sz, sz);
  return sz;
}

auto channel_buffer::prepend(const void* buf, const std::size_t len) noexcept
    -> bool {
  if (prependable() < len) return false;

  std::memmove(static_cast<std::uint8_t*>(data_) + read_ - len, buf, len);
  read_ -= len;
  return true;
}

template class JT_API base_memory_buffer<1024>;
template class JT_API base_memory_buffer<2048>;
template class JT_API base_memory_buffer<4096>;
template class JT_API base_memory_buffer<8192>;

}  // namespace jt::detail