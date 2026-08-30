module;

#include "config.h"

export module jt:detail.buffer;

import std;
import :detail.memory;

export namespace jt::detail {

/**
 * 只读缓冲类
 *
 * 用于从内存区域读取数据，不支持写操作。
 * 维护一个读指针记录已读取的位置。
 */
class read_buffer {
 public:
  /**
   * 构造一个空的只读缓冲区
   */
  constexpr read_buffer() = default;

  /**
   * 构造一个从指定内存区域读取的缓冲区
   * @param ptr 指向数据的指针
   * @param capacity 缓冲区容量
   */
  constexpr read_buffer(const void* ptr, const std::size_t capacity)
      : ptr_(ptr), capacity_(capacity) {}

  /**
   * 构造一个从字符串视图读取的缓冲区
   * @param strv 字符串视图
   */
  constexpr explicit read_buffer(const std::string_view& strv)
      : ptr_(strv.data()), capacity_(strv.size()) {}

  /**
   * 拷贝构造函数
   */
  constexpr read_buffer(const read_buffer&) = default;

  /**
   * 移动构造函数
   * @param other 要移动的缓冲区
   */
  constexpr read_buffer(read_buffer&& other) noexcept
      : ptr_(other.ptr_), capacity_(other.capacity_), read_(other.read_) {
    other.ptr_ = nullptr;
    other.capacity_ = 0;
    other.read_ = 0;
  }

  /**
   * 拷贝赋值运算符
   */
  constexpr auto operator=(const read_buffer&) -> read_buffer& = default;

  /**
   * 移动赋值运算符
   * @param other 要赋值的缓冲区
   * @return 赋值后的缓冲区
   */
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

  /**
   * 获取读取起始位置
   * @return 指向第一个可读字节的指针
   */
  [[nodiscard]] constexpr auto begin() const -> const std::uint8_t* {
    return static_cast<const std::uint8_t*>(ptr_) + read_;
  }

  /**
   * 获取读取结束位置
   * @return 指向缓冲区末尾的指针
   */
  [[nodiscard]] constexpr auto end() const -> const std::uint8_t* {
    return static_cast<const std::uint8_t*>(ptr_) + capacity_;
  }

  /**
   * 获取原始数据指针
   * @return 指向缓冲区起始位置的指针
   */
  [[nodiscard]] constexpr auto data() const -> const void* { return ptr_; }

  /**
   * 获取缓冲区容量
   * @return 缓冲区总容量（字节）
   */
  [[nodiscard]] constexpr auto capacity() const -> std::size_t {
    return capacity_;
  }

  /**
   * 获取可读字节数
   * @return 可读取的数据长度
   */
  [[nodiscard]] constexpr auto readable() const -> std::size_t {
    return capacity_ - read_;
  }

  /**
   * 读取数据到目标缓冲区
   * @param dest 目标缓冲区
   * @param size 请求读取的字节数
   * @return 实际读取的字节数
   */
  auto read(void* dest, std::size_t size) -> std::size_t {  // NOLINT
    size = (std::min)(readable(), size);
    std::memcpy(dest, begin(), size);
    read_ += size;
    return size;
  }

  /**
   * 跳过指定字节数
   * @param bytes 要跳过的字节数
   * @return 引用自身
   */
  constexpr auto operator+=(const std::size_t bytes) noexcept -> read_buffer& {
    read_ = (std::min)(read_ + bytes, capacity_);
    return *this;
  }

  /**
   * 转换为字符串视图
   * @return 字符串视图，如果无可读数据则为空
   */
  constexpr explicit operator std::string_view() const noexcept {
    if (const auto sz = readable(); sz > 0) {
      return {reinterpret_cast<const char*>(begin()), sz};
    }

    return {};
  }

 private:
  /**
   * 数据指针
   */
  const void* ptr_{nullptr};
  /**
   * 缓冲区总容量
   */
  std::size_t capacity_{0};
  /**
   * 已读取的字节数
   */
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
  using value_type = std::uint8_t;

  /**
   * 构造一个空的读写缓冲区
   */
  constexpr channel_buffer() = default;

  /**
   * 构造一个从指定内存区域读写的缓冲区
   * @param ptr 指向数据的指针
   * @param capacity 缓冲区总容量
   * @param prependable 前置可写区域大小，默认为0
   */
  constexpr channel_buffer(void* ptr, const std::size_t capacity,
                           const std::size_t prependable = 0)
      : data_(ptr), capacity_(capacity) {
    if (prependable <= capacity) {
      read_ = prependable;
      write_ = prependable;
    }
  }

  /**
   * 拷贝构造函数（删除，不支持拷贝）
   */
  channel_buffer(const channel_buffer&) = delete;

  /**
   * 移动构造函数
   * @param other 要移动的缓冲区
   */
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

  /**
   * 拷贝赋值运算符（删除，不支持拷贝）
   */
  auto operator=(const channel_buffer&) -> channel_buffer& = delete;

  /**
   * 移动赋值运算符
   * @param other 要赋值的缓冲区
   * @return 赋值后的缓冲区
   */
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

  /**
   * 获取可读区域起始位置
   * @return 指向第一个可读字节的指针
   */
  [[nodiscard]] constexpr auto begin_read() const -> const std::uint8_t* {
    return static_cast<const std::uint8_t*>(data_) + read_;
  }

  /**
   * 获取可读区域结束位置
   * @return 指向最后一个可读字节后一个位置的指针
   */
  [[nodiscard]] constexpr auto end_read() const -> const std::uint8_t* {
    return static_cast<const std::uint8_t*>(data_) + write_;
  }

  template <typename Self>
  /**
   * 获取可写区域起始位置
   * @return 指向第一个可写字节的指针
   */
  [[nodiscard]] constexpr auto begin(this Self&& self) {
    return static_cast<std::uint8_t*>(self.data_) + self.write_;
  }

  template <typename Self>
  /**
   * 获取可写区域结束位置
   * @return 指向缓冲区末尾的指针
   */
  [[nodiscard]] constexpr auto end(this Self&& self) {
    return static_cast<std::uint8_t*>(self.data_) + self.capacity_;
  }

  template <typename Self>
  /**
   * 获取原始数据指针
   * @return 指向缓冲区起始位置的指针
   */
  [[nodiscard]] constexpr auto data(this Self&& self) {
    return self.data_;
  }

  /**
   * 获取可读字节数
   * @return 可读取的数据长度
   */
  [[nodiscard]] constexpr auto readable() const -> std::size_t {
    return write_ - read_;
  }

  /**
   * 获取可写字节数
   * @return 可写入的数据长度
   */
  [[nodiscard]] constexpr auto writable() const -> std::size_t {
    return capacity_ - write_;
  }

  /**
   * 获取缓冲区总容量
   * @return 缓冲区总容量
   */
  [[nodiscard]] constexpr auto capacity() const -> std::size_t {
    return capacity_;
  }

  /**
   * 获取前置可写区域大小
   * @return 前置可写区域大小
   */
  [[nodiscard]] constexpr auto prependable() const -> std::size_t {
    return read_;
  }

  /**
   * 收缩缓冲区，丢弃已读取的数据
   */
  void shrink() noexcept;

  /**
   * 转换为只读缓冲区
   * @return 只读缓冲区
   */
  constexpr explicit operator read_buffer() const noexcept {
    const auto size = readable();
    if (size == 0) return {};

    return {begin_read(), size};
  }

  /**
   * 转换为字符串视图
   * @return 字符串视图，如果无可读数据则为空
   */
  constexpr explicit operator std::string_view() const noexcept {
    const auto size = readable();
    if (size == 0) return {};

    return {reinterpret_cast<const char*>(begin_read()), size};
  }

  /**
   * 清空缓冲区
   * @param prependable 重新设置的前置可写区域大小
   */
  constexpr void clear(std::size_t prependable = 0) noexcept {
    prependable = (std::min)(prependable, capacity_);
    read_ = prependable;
    write_ = prependable;
  }

  /**
   * 追加数据到缓冲区
   * @param buf 数据指针
   * @param len 数据长度
   */
  void append(const void* buf, std::size_t len);

  /**
   * 追加只读缓冲区数据
   * @param buf 只读缓冲区
   */
  void append(const read_buffer& buf);

  /**
   * 追加字符串视图
   * @param strv 字符串视图
   */
  void append(std::string_view strv);

  /**
   * 追加C字符串
   * @param str C字符串
   */
  void append(const char* str);

  /**
   * 追加单个字节
   * @param val 要追加的字节值
   */
  void push_back(std::uint8_t val);

  /**
   * 查看数据（不移动读指针）
   * @param buf 目标缓冲区
   * @param sz 请求读取的字节数
   * @return 实际读取的字节数
   */
  [[nodiscard]] auto peek(void* buf, std::size_t sz) const noexcept
      -> std::size_t;

  /**
   * 反向查看数据（从末尾读取，不移动读指针）
   * @param buf 目标缓冲区
   * @param sz 请求读取的字节数
   * @return 实际读取的字节数
   */
  [[nodiscard]] auto rpeek(void* buf, std::size_t sz) const noexcept
      -> std::size_t;

  /**
   * 在前端 prepend 数据
   * @param buf 数据指针
   * @param len 数据长度
   * @return 是否成功
   */
  auto prepend(const void* buf, std::size_t len) noexcept -> bool;

  /**
   * 标记写入指定字节数
   * @param len 写入的字节数
   */
  constexpr void written(const std::size_t len) noexcept {
    write_ += (std::min)(len, writable());
  }

  /**
   * 读取指定字节数
   * @param len 读取的字节数
   */
  constexpr void read(const std::size_t len) noexcept {
    read_ += (std::min)(len, readable());
  }

  /**
   * 读取直到指定位置
   * @param end 读取结束位置
   */
  constexpr void read_until(const std::uint8_t* end) noexcept {  // NOLINT
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
/**
 * 固定大小内存缓冲区模板类
 *
 * 继承自 channel_buffer，使用固定大小的栈内存作为存储。
 * 当容量不足时会动态分配堆内存。
 *
 * @tparam Fixed 固定栈内存大小（字节）
 */
class base_memory_buffer : public channel_buffer {
 public:
  static_assert(Fixed > 0, "Fixed must > 0");

  /**
   * 构造一个指定前置可写区域大小的缓冲区
   * @param prependable 前置可写区域大小，默认为0
   */
  explicit base_memory_buffer(const std::size_t prependable = 0)
      : channel_buffer(store_, Fixed, prependable), using_heap_(false) {}

  /**
   * 拷贝构造函数
   * @param other 要拷贝的缓冲区
   */
  base_memory_buffer(const base_memory_buffer& other) : base_memory_buffer(0) {
    reserve(other.capacity_);
    read_ = other.read_;
    write_ = other.write_;
    std::memcpy(data_, other.data_, write_);
  }

  template <std::size_t FixedOther>
  /**
   * 从不同大小的 base_memory_buffer 拷贝构造
   * @tparam FixedOther 其他模板实例的固定大小
   * @param other 要拷贝的缓冲区
   */
  explicit base_memory_buffer(const base_memory_buffer<FixedOther>& other)
      : base_memory_buffer(0) {
    reserve(other.capacity_);
    read_ = other.read_;
    write_ = other.write_;
    std::memcpy(data_, other.data_, write_);
  }

  /**
   * 移动构造函数
   * @param other 要移动的缓冲区
   */
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

  /**
   * 从只读缓冲区构造
   * @param buf 只读缓冲区
   */
  explicit base_memory_buffer(const read_buffer& buf) { append(buf); }

  /**
   * 从内存区域构造
   * @param ptr 数据指针
   * @param len 数据长度
   */
  base_memory_buffer(const void* ptr, const std::size_t len) {
    append(ptr, len);
  }

  /**
   * 析构函数
   */
  ~base_memory_buffer() noexcept {
    // ReSharper disable once CppDFAConstantConditions
    if (using_heap_) {
      // ReSharper disable once CppDFAUnreachableCode
      deallocate(data_);
    }
  }

  /**
   * 确保缓冲区至少有指定容量
   * @param size 最小容量要求
   */
  void reserve(const std::size_t size) {
    if (size > capacity_) {
      grow(size);
    }
  }

  /**
   * 释放堆内存，返回固定大小栈内存
   */
  void release() {
    if (using_heap_) {
      deallocate(data_);
      data_ = store_;
      capacity_ = Fixed;
      using_heap_ = false;
    }

    clear();
  }

  /**
   * 确保可写入指定字节数，不足时扩容
   * @param len 需要的可写字节数
   */
  void make_sure_writable(const std::size_t len) {
    if (const auto sz = writable(); sz < len) {
      grow(capacity_ + len - sz);
    }
  }

  /**
   * 拷贝赋值运算符
   * @param other 要赋值的缓冲区
   * @return 赋值后的缓冲区
   */
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
  /**
   * 从不同大小的 base_memory_buffer 拷贝赋值
   * @tparam FixedOther 其他模板实例的固定大小
   * @param other 要赋值的缓冲区
   * @return 赋值后的缓冲区
   */
  auto operator=(const base_memory_buffer<FixedOther>& other)
      -> base_memory_buffer& {
    reserve(other.capacity_);
    read_ = other.read_;
    write_ = other.write_;
    std::memcpy(data_, other.data_, write_);
    return *this;
  }

  /**
   * 移动赋值运算符
   * @param other 要赋值的缓冲区
   * @return 赋值后的缓冲区
   */
  auto operator=(base_memory_buffer&& other) noexcept -> base_memory_buffer& {
    if (this != std::addressof(other)) {
      if (using_heap_) {
        deallocate(data_);
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

  /**
   * 追加数据到缓冲区
   * @param buf 数据指针
   * @param len 数据长度
   */
  void append(const void* buf, const std::size_t len) {
    make_sure_writable(len);
    return channel_buffer::append(buf, len);
  }

  /**
   * 追加字符串视图
   * @param strv 字符串视图
   */
  void append(const std::string_view strv) {
    return append(strv.data(), strv.size());
  }

  /**
   * 追加只读缓冲区数据
   * @param buf 只读缓冲区
   */
  void append(const read_buffer& buf) {
    return append(buf.begin(), buf.readable());
  }

  /**
   * 追加C字符串
   * @param str C字符串
   */
  void append(const char* str) { return append(str, std::strlen(str)); }

  /**
   * 追加单个字节
   * @param val 要追加的字节值
   */
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
  /**
   * 扩容缓冲区
   * 当可写入空间不足时，扩容为原容量的1.5倍或所需大小
   * @param size 新容量
   */
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
      deallocate(old_data);
    }

    data_ = new_data;
    capacity_ = new_capacity;
    using_heap_ = true;
  }

  alignas(std::max_align_t) std::uint8_t store_[Fixed]{};
  bool using_heap_{false};
};

extern template class base_memory_buffer<1024>;
extern template class base_memory_buffer<2048>;
extern template class base_memory_buffer<4096>;
extern template class base_memory_buffer<8192>;

using buffer_1k = base_memory_buffer<1024>;
using buffer_2k = base_memory_buffer<2048>;
using buffer_4k = base_memory_buffer<4096>;
using buffer_8k = base_memory_buffer<8192>;

}  // namespace jt::detail
