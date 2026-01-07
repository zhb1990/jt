module;

#include "config.h"

export module jt:detail.buffer;

import :detail.memory;
import std;

export namespace jt::detail {

class read_buffer {
 public:
  constexpr read_buffer() = default;

  constexpr read_buffer(const void* ptr, const std::size_t capacity)
      : ptr_(ptr), capacity_(capacity) {}

  constexpr explicit read_buffer(const std::string_view& strv)
      : ptr_(strv.data()), capacity_(strv.size()) {}

  constexpr read_buffer(const read_buffer&) = default;

  constexpr read_buffer(read_buffer&& other) noexcept
      : ptr_(other.ptr_), capacity_(other.capacity_), read_(other.read_) {
    other.ptr_ = nullptr;
    other.capacity_ = 0;
    other.read_ = 0;
  }

  constexpr auto operator=(const read_buffer&) -> read_buffer& = default;

  constexpr auto operator=(read_buffer&& other) noexcept -> read_buffer& {
    if (this != std::addressof(other)) {
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      capacity_ = other.capacity_;
      other.capacity_ = 0;
      read_ = other.read_;
      other.read_ = 0;
    }

    return *this;
  }

  [[nodiscard]] constexpr auto begin() const -> const std::uint8_t* {
    return static_cast<const std::uint8_t*>(ptr_) + read_;
  }

  [[nodiscard]] constexpr auto end() const -> const std::uint8_t* {
    return static_cast<const std::uint8_t*>(ptr_) + capacity_;
  }

  [[nodiscard]] constexpr auto data() const -> const void* { return ptr_; }

  [[nodiscard]] constexpr auto capacity() const -> std::size_t {
    return capacity_;
  }

  [[nodiscard]] constexpr auto readable() const -> std::size_t {
    return capacity_ - read_;
  }

  auto read(void* dest, std::size_t size) -> std::size_t {
    size = (std::min)(readable(), size);
    std::memcpy(dest, begin(), size);
    read_ += size;
    return size;
  }

  constexpr auto operator+=(const std::size_t bytes) noexcept -> read_buffer& {
    read_ = (std::min)(read_ + bytes, capacity_);
    return *this;
  }

  constexpr explicit operator std::string_view() const noexcept {
    if (const auto sz = readable(); sz > 0) {
      return {reinterpret_cast<const char*>(begin()), sz};
    }

    return {};
  }

 private:
  const void* ptr_{nullptr};
  std::size_t capacity_{0};
  std::size_t read_{0};
};

/**
 * 读写缓冲区
 *
 * 修改自 org.jboss.netty.buffer.ChannelBuffer
 * @code
 * +-------------------+------------------+------------------+
 * | PrependAble bytes |  Readable bytes  |  Writable bytes  |
 * |                   |     (CONTENT)    |                  |
 * +-------------------+------------------+------------------+
 * |                   |                  |                  |
 * 0         <=      read_        <=   write_    <=     capacity_
 *
 * @endcode
 * 缓冲区被划分为三个部分：
 *
 * - PrependAble Bytes（前置可写区域）:
 *   起始位置：0 到
 * read_。这部分空间可以用来在现有内容之前插入少量数据（例如协议头）。
 *
 * - Readable Bytes（可读区域，CONTENT）:
 *   起始位置：read_ 到 write_。这是缓冲区的核心部分，存储当前可以读取的数据。
 *
 * - Writable Bytes（可写区域）:
 *   起始位置：write_ 到 capacity_。这部分空间用于写入新数据。
 */
class JT_API channel_buffer {
 public:
  constexpr channel_buffer() = default;

  constexpr channel_buffer(void* ptr, const std::size_t capacity,
                           const std::size_t prependable = 0)
      : data_(ptr), capacity_(capacity) {
    if (prependable <= capacity) {
      read_ = prependable;
      write_ = prependable;
    }
  }

  channel_buffer(const channel_buffer&) = delete;

  channel_buffer(channel_buffer&& other) noexcept
      : data_(other.data_),
        capacity_(other.capacity_),
        read_(other.read_),
        write_(other.write_) {
    other.data_ = nullptr;
    other.capacity_ = 0;
    other.read_ = 0;
    other.write_ = 0;
  }

  auto operator=(const channel_buffer&) -> channel_buffer& = delete;

  constexpr auto operator=(channel_buffer&& other) noexcept -> channel_buffer& {
    if (this != std::addressof(other)) {
      data_ = other.data_;
      other.data_ = nullptr;
      capacity_ = other.capacity_;
      other.capacity_ = 0;
      read_ = other.read_;
      other.read_ = 0;
      write_ = other.write_;
      other.write_ = 0;
    }

    return *this;
  }

  [[nodiscard]] constexpr auto begin_read() const -> const std::uint8_t* {
    return static_cast<const std::uint8_t*>(data_) + read_;
  }

  [[nodiscard]] constexpr auto end_read() const -> const std::uint8_t* {
    return static_cast<const std::uint8_t*>(data_) + write_;
  }

  template <typename Self>
  [[nodiscard]] constexpr auto begin(this Self&& self) {
    return static_cast<std::uint8_t*>(self.data_) + self.write_;
  }

  template <typename Self>
  [[nodiscard]] constexpr auto end(this Self&& self) {
    return static_cast<std::uint8_t*>(self.data_) + self.capacity_;
  }

  template <typename Self>
  [[nodiscard]] constexpr auto data(this Self&& self) {
    return self.data_;
  }

  [[nodiscard]] constexpr auto readable() const -> std::size_t {
    return write_ - read_;
  }

  [[nodiscard]] constexpr auto writable() const -> std::size_t {
    return capacity_ - write_;
  }

  [[nodiscard]] constexpr auto capacity() const -> std::size_t {
    return capacity_;
  }

  [[nodiscard]] constexpr auto prependable() const -> std::size_t {
    return read_;
  }

  void shrink() noexcept {
    if (read_ == 0) return;

    const auto size = readable();
    std::memmove(data_, begin_read(), size);
    read_ = 0;
    write_ = size;
  }

  constexpr explicit operator read_buffer() const noexcept {
    const auto size = readable();
    if (size == 0) return {};

    return {begin_read(), size};
  }

  constexpr explicit operator std::string_view() const noexcept {
    const auto size = readable();
    if (size == 0) return {};

    return {reinterpret_cast<const char*>(begin_read()), size};
  }

  constexpr void clear(std::size_t prependable = 0) noexcept {
    prependable = (std::min)(prependable, capacity_);
    read_ = prependable;
    write_ = prependable;
  }

  void append(const void* buf, std::size_t len) {
    len = (std::min)(len, writable());
    std::memmove(begin(), buf, len);
    written(len);
  }

  void append(const read_buffer& buf) {
    return append(buf.begin(), buf.readable());
  }

  void append(const std::string_view strv) {
    return append(strv.data(), strv.size());
  }

  void append(const char* str) { return append(str, std::strlen(str)); }

  [[nodiscard]] auto peek(void* buf, std::size_t sz) const noexcept
      -> std::size_t {
    sz = (std::min)(sz, readable());
    std::memmove(buf, begin_read(), sz);
    return sz;
  }

  [[nodiscard]] auto rpeek(void* buf, std::size_t sz) const noexcept
      -> std::size_t {
    sz = (std::min)(sz, readable());
    std::memmove(buf, end_read() - sz, sz);
    return sz;
  }

  auto prepend(const void* buf, const std::size_t len) noexcept -> bool {
    if (prependable() < len) return false;

    std::memmove(static_cast<std::uint8_t*>(data_) + read_ - len, buf, len);
    read_ -= len;
    return true;
  }

  constexpr void written(const std::size_t len) noexcept {
    write_ += (std::min)(len, writable());
  }

  constexpr void read(const std::size_t len) noexcept {
    read_ += (std::min)(len, readable());
  }

  constexpr void read_until(const std::uint8_t* end) noexcept {
    if (end > begin_read() && end <= end_read()) {
      read_ += end - begin_read();
    }
  }

 protected:
  void* data_{nullptr};
  std::size_t capacity_{0};
  std::size_t read_{0};
  std::size_t write_{0};
};

template <std::size_t Fixed>
class base_memory_buffer : public channel_buffer {
 public:
  static_assert(Fixed > 0, "Fixed must > 0");

  using value_type = std::uint8_t;

  explicit base_memory_buffer(const std::size_t prependable = 0)
      : channel_buffer(store_, Fixed, prependable), using_heap_(false) {}

  base_memory_buffer(const base_memory_buffer& other) : base_memory_buffer(0) {
    reserve(other.capacity_);
    read_ = other.read_;
    write_ = other.write_;
    std::memcpy(data_, other.data_, write_);
  }

  template <std::size_t FixedOther>
  explicit base_memory_buffer(const base_memory_buffer<FixedOther>& other)
      : base_memory_buffer(0) {
    reserve(other.capacity_);
    read_ = other.read_;
    write_ = other.write_;
    std::memcpy(data_, other.data_, write_);
  }

  base_memory_buffer(base_memory_buffer&& other) noexcept {
    if (other.using_heap_) {
      data_ = other.data_;
      using_heap_ = true;
      capacity_ = other.capacity_;

      other.data_ = other.store_;
      other.using_heap_ = false;
      other.capacity_ = Fixed;

    } else {
      std::memcpy(store_, other.store_, other.write_);
      data_ = store_;
      capacity_ = Fixed;
      using_heap_ = false;
    }

    read_ = other.read_;
    write_ = other.write_;
    other.read_ = 0;
    other.write_ = 0;
  }

  explicit base_memory_buffer(const read_buffer& buf) { append(buf); }

  base_memory_buffer(const void* ptr, const std::size_t len) {
    append(ptr, len);
  }

  ~base_memory_buffer() noexcept {
    if (using_heap_) {
      deallocate(data_, capacity_);
    }
  }

  void reserve(const std::size_t size) {
    if (size > capacity_) {
      grow(size);
    }
  }

  void release() {
    if (using_heap_) {
      deallocate(data_, capacity_);
      data_ = store_;
      capacity_ = Fixed;
      using_heap_ = false;
    }

    clear();
  }

  void make_sure_writable(const std::size_t len) {
    if (const auto sz = writable(); sz < len) {
      grow(capacity_ + len - sz);
    }
  }

  auto operator=(const base_memory_buffer& other) -> base_memory_buffer& {
    if (this != &other) {
      reserve(other.capacity_);
      read_ = other.read_;
      write_ = other.write_;
      std::memcpy(data_, other.data_, write_);
    }

    return *this;
  }

  template <std::size_t FixedOther>
  auto operator=(const base_memory_buffer<FixedOther>& other)
      -> base_memory_buffer& {
    reserve(other.capacity_);
    read_ = other.read_;
    write_ = other.write_;
    std::memcpy(data_, other.data_, write_);
    return *this;
  }

  auto operator=(base_memory_buffer&& other) noexcept -> base_memory_buffer& {
    if (this != std::addressof(other)) {
      if (using_heap_) {
        deallocate(data_, capacity_);
      }

      if (other.using_heap_) {
        data_ = other.data_;
        capacity_ = other.capacity_;
        using_heap_ = true;
        other.data_ = other.store_;
        other.capacity_ = Fixed;
        other.using_heap_ = false;
      } else {
        data_ = store_;
        capacity_ = Fixed;
        std::memcpy(store_, other.store_, other.write_);
        using_heap_ = false;
      }

      read_ = other.read_;
      write_ = other.write_;
      other.read_ = 0;
      other.write_ = 0;
    }

    return *this;
  }

  void append(const void* buf, const std::size_t len) {
    make_sure_writable(len);
    return channel_buffer::append(buf, len);
  }

  void append(const std::string_view strv) {
    return append(strv.data(), strv.size());
  }

  void append(const read_buffer& buf) {
    return append(buf.begin(), buf.readable());
  }

  void append(const char* str) { return append(str, std::strlen(str)); }

  void push_back(const std::uint8_t val) { return append(&val, sizeof(val)); }

  using channel_buffer::begin;
  using channel_buffer::begin_read;
  using channel_buffer::capacity;
  using channel_buffer::data;
  using channel_buffer::end;
  using channel_buffer::end_read;
  using channel_buffer::prependable;
  using channel_buffer::readable;
  using channel_buffer::shrink;
  using channel_buffer::writable;
  using channel_buffer::operator read_buffer;
  using channel_buffer::operator std::string_view;
  using channel_buffer::clear;
  using channel_buffer::peek;
  using channel_buffer::prepend;
  using channel_buffer::read;
  using channel_buffer::read_until;
  using channel_buffer::rpeek;
  using channel_buffer::written;

 private:
  void grow(const std::size_t size) {
    constexpr auto max_size = static_cast<std::size_t>(-1);
    std::size_t new_capacity = capacity_ + capacity_ / 2;
    if (size > new_capacity) {
      new_capacity = size;
    } else if (new_capacity > max_size) {
      new_capacity = size > max_size ? size : max_size;
    }

    auto* old_data = data_;
    const auto old_capacity = capacity_;

    void* new_data = allocate(new_capacity);
    std::memcpy(new_data, old_data, write_);
    if (using_heap_) {
      deallocate(old_data, old_capacity);
    }

    data_ = new_data;
    capacity_ = new_capacity;
    using_heap_ = true;
  }

  alignas(std::max_align_t) std::uint8_t store_[Fixed]{};
  bool using_heap_{false};
};

using buffer_1k = base_memory_buffer<1024>;
using buffer_2k = base_memory_buffer<2048>;
using buffer_4k = base_memory_buffer<4096>;
using buffer_8k = base_memory_buffer<8192>;

}  // namespace jt::detail
