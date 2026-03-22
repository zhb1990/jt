export module jt:detail.metric_value;

import std;
import :detail.cache_line;

export namespace jt::detail {

/**
 * 线程局部计数器
 * 用于高效统计多线程场景下的累加值
 * 通过为每个线程分配独立的存储空间来避免锁竞争
 */
class metric_value {
 public:
  /**
   * 构造函数
   * 根据硬件并发线程数分配存储空间
   */
  metric_value() : size_(std::thread::hardware_concurrency()) {
    datas_ = new data[size_];
  }

  /**
   * 析构函数
   * 释放分配的存储空间
   */
  ~metric_value() { delete[] datas_; }

  // 删除拷贝构造函数和赋值运算符
  metric_value(metric_value&) = delete;
  metric_value(metric_value&&) = delete;
  metric_value& operator=(metric_value&) = delete;
  metric_value& operator=(metric_value&&) = delete;

  /**
   * 原子加法操作
   * @param val 要加的值
   * @return 加法之前的值
   */
  auto fetch_add(const std::int64_t val) -> std::int64_t {
    return get_local_count().fetch_add(val, std::memory_order::relaxed);
  }

  /**
   * 原子减法操作
   * @param val 要减的值
   * @return 减法之前的值
   */
  auto fetch_sub(const std::int64_t val) -> std::int64_t {
    return get_local_count().fetch_sub(val, std::memory_order::relaxed);
  }

  /**
   * 获取总计数值
   * 遍历所有线程的局部计数并求和
   * @return 所有线程计数的总和
   */
  [[nodiscard]] auto count()  // NOLINT(*-convert-member-functions-to-static)
      const -> std::int64_t {
    std::int64_t cnt = 0;
    for (std::uint32_t i = 0; i < size_; ++i) {
      cnt += this->datas_[i].value.load(std::memory_order::relaxed);
    }
    return cnt;
  }

 private:
  /**
   * 获取当前线程对应的局部计数
   * 使用thread_local确保每个线程访问不同的存储空间
   * 通过round-robin方式分配线程到不同的槽位
   * @return 当前线程对应的原子计数引用
   */
  // ReSharper disable once CppMemberFunctionMayBeConst
  std::atomic_int64_t& get_local_count() {
    static std::atomic_uint32_t round{0};  // 用于轮询分配线程到槽位
    thread_local std::uint32_t index = round++ % size_;
    return datas_[index].value;
  }

  /**
   * 数据结构：对齐到缓存行大小以避免伪共享
   * 每个数据项包含一个原子64位整数
   */
  struct alignas(cache_line_bytes) data {
    std::atomic_int64_t value{0};
  };

  std::uint32_t size_{0};  // 槽位数量（通常等于硬件并发线程数）
  data* datas_{nullptr};   // 动态分配的数据数组
};

}  // namespace jt::detail
