import std;
import jt;
namespace mem = jt::base;

struct counter_sink final : jt::log::sink {
  explicit counter_sink(std::atomic<std::size_t>& count) : count(count) {}
  void write(jt::log::level, const time_point&, const mem::buffer_1k&,
             std::size_t, std::size_t) override {
    count.fetch_add(1, std::memory_order_relaxed);
  }
  void flush_unlock() override {}
  std::atomic<std::size_t>& count;
};

int main() {
  constexpr std::size_t n = 20000;
  for (bool async : {false, true}) {
    std::atomic<std::size_t> count{0};
    std::vector<std::int64_t> latency(n);
    const auto memory_before = mem::allocated_memory();
    const auto start = std::chrono::steady_clock::now();
    std::int64_t pending_bytes = 0;
    {
      jt::log::service service;
      auto log = service.create_logger(
          std::array{
              mem::make_dynamic_unique<jt::log::sink, counter_sink>(count)},
          "bench", async);
      for (std::size_t i = 0; i < n; ++i) {
        const auto begin = std::chrono::steady_clock::now();
        jt::log::info(*log, "benchmark {}", i);
        latency[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::steady_clock::now() - begin)
                         .count();
      }
      pending_bytes = mem::allocated_memory() - memory_before;
    }
    const auto elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
            .count();
    std::ranges::sort(latency);
    std::println(
        "{} count={} throughput={:.0f}/s p50={}ns p99={}ns pending_bytes={} "
        "retained_bytes={}",
        async ? "async" : "sync", count.load(), n / elapsed, latency[n / 2],
        latency[n * 99 / 100], pending_bytes,
        mem::allocated_memory() - memory_before);
    if (count != n) return 1;
  }
}
