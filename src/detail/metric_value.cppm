export module jt:detail.metric_value;

import std;
import :detail.cache_line;

export namespace jt::detail {

class metric_value {
 public:
  metric_value() : size_(std::thread::hardware_concurrency()) {
    datas_ = new data[size_];
  }

  ~metric_value() { delete[] datas_; }

  metric_value(metric_value&) = delete;
  metric_value(metric_value&&) = delete;
  metric_value& operator=(metric_value&) = delete;
  metric_value& operator=(metric_value&&) = delete;

  auto fetch_add(const std::int64_t val) -> std::int64_t {
    return get_local_count().fetch_add(val, std::memory_order::relaxed);
  }

  auto fetch_sub(const std::int64_t val) -> std::int64_t {
    return get_local_count().fetch_sub(val, std::memory_order::relaxed);
  }

  [[nodiscard]] auto count()  // NOLINT(*-convert-member-functions-to-static)
      const -> std::int64_t {
    std::int64_t cnt = 0;
    for (std::uint32_t i = 0; i < size_; ++i) {
      cnt += this->datas_[i].value.load(std::memory_order::relaxed);
    }
    return cnt;
  }

 private:
  // ReSharper disable once CppMemberFunctionMayBeConst
  std::atomic_int64_t& get_local_count() {
    static std::atomic_uint32_t round{0};
    thread_local std::uint32_t index = round++ % size_;
    return datas_[index].value;
  }

  struct data {
    std::atomic_int64_t value{0};
    char padding[cache_line_bytes - sizeof(std::atomic_int64_t)];
  };

  std::uint32_t size_{0};
  data* datas_{nullptr};
};

}  // namespace jt::detail