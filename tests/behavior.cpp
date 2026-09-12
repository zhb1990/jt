import std;
import jt;

namespace mem = jt::base;

void check(bool condition, std::string_view message) {
  if (!condition) throw std::runtime_error(std::string(message));
}

struct state {
  std::mutex mutex;
  std::vector<std::string> lines;
  std::atomic<int> destroyed{0};
  std::atomic<int> formatter_destroyed{0};
  std::atomic<int> flushes{0};
  std::size_t flushed_lines{0};
  std::uint_least32_t source_line{0};
};

struct capture_formatter final : jt::log::formatter {
  explicit capture_formatter(state& s) : s(s) {}
  ~capture_formatter() noexcept override { ++s.formatter_destroyed; }
  void format(const jt::log::log_record_view& record, mem::buffer_1k& output,
              std::size_t& start, std::size_t& stop) override {
    output.append(record.payload);
    s.source_line = record.source.line();
    start = stop = 0;
  }
  state& s;
};

struct capture_sink final : jt::log::sink {
  explicit capture_sink(state& s) : s(s) {
    auto fmt =
        mem::make_dynamic_unique<jt::log::formatter, capture_formatter>(s);
    set_formatter(std::move(fmt));
  }
  ~capture_sink() noexcept override { ++s.destroyed; }
  void write(jt::log::level, const time_point&, const mem::buffer_1k& buffer,
             std::size_t, std::size_t) override {
    std::lock_guard lock(s.mutex);
    s.lines.emplace_back(std::string_view(buffer));
  }
  void flush_unlock() override {
    s.flushed_lines = s.lines.size();
    ++s.flushes;
  }
  state& s;
};

auto make_logger(jt::log::service& service, state& s, bool async) {
  auto sink = mem::make_dynamic_unique<jt::log::sink, capture_sink>(s);
  return service.create_logger(std::array{std::move(sink)}, "capture", async);
}

void memory_and_buffer() {
  const auto before = mem::allocated_memory();
  {
    auto* ptr = mem::allocate(123);
    check(mem::allocated_size(ptr) >= 123, "allocation size");
    mem::deallocate(ptr);
    struct throwing {
      throwing() { throw std::runtime_error("expected"); }
    };
    try {
      auto p = mem::make_unique<throwing>();
    } catch (const std::runtime_error&) {
    }
    mem::base_memory_buffer<4> buffer;
    buffer.append("abc");
    auto copy = buffer;
    buffer.append("defgh");
    check(std::string_view(copy) == "abc", "inline copy");
    check(std::string_view(buffer) == "abcdefgh", "heap growth");
    auto moved = std::move(buffer);
    check(buffer.readable() == 0, "moved source");
    moved.read(3);
    moved.shrink();
    check(std::string_view(moved) == "defgh", "read and compact");
    copy = moved;
    check(std::string_view(copy) == "defgh", "heap copy");
    moved.release();
    check(moved.capacity() == 4 && moved.readable() == 0, "release inline");
    mem::read_buffer reader(copy);
    char bytes[3]{};
    check(reader.read(bytes, 2) == 2 && std::string_view(bytes) == "de",
          "read view");
    mem::buffer_1k b1;
    mem::buffer_2k b2;
    mem::buffer_4k b4;
    mem::buffer_8k b8;
    b1.append("1");
    b2.append("2");
    b4.append("4");
    b8.append("8");
    mem::vector<int> values{1, 2, 3};
    check(values.size() == 3, "allocator container");
  }
  check(mem::allocated_memory() == before,
        "memory balance and throwing constructor cleanup");
}

void buffer_self_append() {
  const auto before = mem::allocated_memory();
  {
    mem::base_memory_buffer<4> buffer;
    buffer.append("abcdefgh");
    const auto capacity = buffer.capacity();
    buffer.append(std::string_view(buffer));
    check(buffer.capacity() > capacity, "self append grows heap storage");
    check(std::string_view(buffer) == "abcdefghabcdefgh" &&
              buffer.readable() == 16 && buffer.prependable() == 0,
          "heap self append preserves all bytes");
  }
  {
    mem::base_memory_buffer<4> buffer;
    buffer.append("abcdefgh");
    buffer.read(2);
    const auto capacity = buffer.capacity();
    buffer.append(std::string_view(buffer).substr(1, 4));
    check(buffer.capacity() > capacity && buffer.prependable() == 2 &&
              buffer.readable() == 10 &&
              std::string_view(buffer) == "cdefghdefg",
          "self append relocates a substring after reading");
  }
  {
    mem::base_memory_buffer<4> buffer;
    buffer.append("abcdefgh");
    buffer.read(2);
    const auto capacity = buffer.capacity();
    buffer.append(mem::read_buffer(buffer));
    check(buffer.capacity() > capacity && buffer.prependable() == 2 &&
              std::string_view(buffer) == "cdefghcdefgh",
          "read buffer self append grows heap storage");
  }
  {
    mem::base_memory_buffer<4> buffer;
    buffer.append("abcdefgh");
    buffer.read(3);
    const auto capacity = buffer.capacity();
    // Include consumed bytes: offsets are relative to data(), not begin_read().
    buffer.append(static_cast<const std::uint8_t*>(buffer.data()) + 1, 5);
    check(buffer.capacity() > capacity && buffer.prependable() == 3 &&
              std::string_view(buffer) == "defghbcdef",
          "pointer self append preserves storage-relative offsets");
  }
  {
    mem::base_memory_buffer<4> buffer;
    buffer.append("abc");
    buffer.append(std::string_view(buffer));
    check(buffer.capacity() > 4 && std::string_view(buffer) == "abcabc",
          "self append grows inline storage");
  }
  {
    mem::base_memory_buffer<4> buffer;
    buffer.reserve(32);
    buffer.append("abcdefgh");
    const auto capacity = buffer.capacity();
    buffer.append(std::string_view(buffer));
    check(buffer.capacity() == capacity &&
              std::string_view(buffer) == "abcdefghabcdefgh",
          "self append without growth");
    buffer.append(std::string_view{});
    buffer.append(mem::read_buffer{});
    buffer.append(nullptr, 0);
    check(buffer.capacity() == capacity && buffer.prependable() == 0 &&
              std::string_view(buffer) == "abcdefghabcdefgh",
          "empty append leaves contents unchanged");
  }
  check(mem::allocated_memory() == before, "self append memory balance");
}

void buffer_boundaries() {
  constexpr auto maximum = std::numeric_limits<std::size_t>::max();
  constexpr auto saturated = [] {
    mem::read_buffer reader("abcd");
    reader += 2;
    reader += std::numeric_limits<std::size_t>::max();
    return reader.readable();
  }();
  static_assert(saturated == 0);
  mem::read_buffer reader("abcd");
  reader += 1;
  reader += 1;
  reader += maximum;
  reader += maximum;
  check(reader.readable() == 0, "skip saturates without wrapping");
  mem::base_memory_buffer<4> buffer;
  buffer.append("ab");
  buffer.read(1);
  auto* data = buffer.data();
  bool threw = false;
  try {
    buffer.make_sure_writable(maximum);
  } catch (const std::length_error&) {
    threw = true;
  }
  check(threw && buffer.data() == data && buffer.capacity() == 4 &&
            buffer.prependable() == 1 && buffer.writable() == 2 &&
            std::string_view(buffer) == "b",
        "unrepresentable capacity preserves buffer state");
  buffer.make_sure_writable(8);
  check(buffer.writable() >= 8 && buffer.prependable() == 1 &&
            std::string_view(buffer) == "b",
        "normal growth preserves positions and contents");
}

struct ordered_sink final : jt::log::sink {
  ordered_sink(mem::vector<int>& order, int id) : order(order), id(id) {}
  void write(jt::log::level, const time_point&, const mem::buffer_1k&,
             std::size_t, std::size_t) override {
    order.push_back(id);
  }
  void flush_unlock() override {}
  mem::vector<int>& order;
  int id;
};

void logger_ranges() {
  mem::vector<int> order;
  jt::log::service service;
  auto sinks = std::array{
      mem::make_dynamic_unique<jt::log::sink, ordered_sink>(order, 1),
      mem::make_dynamic_unique<jt::log::sink, ordered_sink>(order, 2)};
  auto range = std::ranges::subrange(std::counted_iterator(sinks.begin(), 2),
                                     std::default_sentinel);
  static_assert(std::ranges::input_range<decltype(range)>);
  static_assert(!std::ranges::common_range<decltype(range)>);
  auto log = service.create_logger(range, "sentinel", false);
  check(!sinks[0] && !sinks[1], "range transfers sink ownership");
  jt::log::info(*log, "ordered");
  check(order == mem::vector<int>{1, 2}, "range preserves sink order");
  auto empty = std::ranges::subrange(std::counted_iterator(sinks.begin(), 0),
                                     std::default_sentinel);
  auto empty_log = service.create_logger(empty, "empty", false);
  jt::log::info(*empty_log, "empty");
  auto common = std::array{
      mem::make_dynamic_unique<jt::log::sink, ordered_sink>(order, 3)};
  auto common_log = service.create_logger(common, "common", false);
  jt::log::info(*common_log, "common");
  check(!common[0] && order == mem::vector<int>{1, 2, 3},
        "common and empty ranges preserve behavior");
}

void synchronous() {
  state s;
  {
    jt::log::service service;
    auto log = make_logger(service, s, false);
    check(service.find("capture") == log, "registry find");
    service.set_default(log);
    check(service.get_default() == log, "default logger");
    log->set_level(jt::log::level::info);
    jt::log::debug(*log, "filtered");
    const auto line = std::source_location::current().line() + 1;
    jt::log::info(*log, "value {}", 42);
    check(s.source_line == line, "source location");
    jt::log::vinfo(*log, "runtime {}", 7);
    log->flush();
    check(s.lines == std::vector<std::string>{"value 42", "runtime 7"},
          "sync filtering and format");
    check(s.flushes == 1, "sync flush");
    service.erase("capture");
    check(!service.find("capture"), "registry erase");
    service.clear();
    check(!service.get_default(), "clear default");
  }
  check(s.destroyed == 1 && s.formatter_destroyed == 1,
        "cross-library virtual destruction");
}

void asynchronous() {
  state s;
  std::shared_ptr<jt::log::logger> log;
  {
    jt::log::service service;
    log = make_logger(service, s, true);
    std::vector<std::jthread> producers;
    for (int producer = 0; producer < 4; ++producer) {
      producers.emplace_back([&, producer] {
        for (int i = 0; i < 500; ++i) jt::log::info(*log, "{}:{}", producer, i);
      });
    }
    producers.clear();
    log->flush();
  }
  check(s.lines.size() == 2000 && s.flushes == 1, "async drain and flush");
  std::set<std::string> unique(s.lines.begin(), s.lines.end());
  check(unique.size() == 2000, "no duplicates");
  for (int producer = 0; producer < 4; ++producer) {
    int expected = 0;
    for (const auto& line : s.lines) {
      if (line.starts_with(std::format("{}:", producer))) {
        check(line == std::format("{}:{}", producer, expected++),
              "per-producer FIFO");
      }
    }
  }
  jt::log::info(*log, "after service destruction");
  log->flush();
  check(s.lines.size() == 2000, "expired backend");
  log.reset();
  check(s.destroyed == 1, "retained logger destruction");
}

void shutdown_flush() {
  state first, second;
  std::shared_ptr<jt::log::logger> first_log, second_log;
  {
    jt::log::service service;
    first_log = make_logger(service, first, true);
    jt::log::info(*first_log, "already consumed");
    // A later explicit flush is a FIFO barrier for the first logger's write.
    second_log = make_logger(service, second, true);
    second_log->flush();
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (second.flushes.load() == 0 &&
           std::chrono::steady_clock::now() < deadline) {
      std::this_thread::yield();
    }
    check(second.flushes == 1, "writer reached FIFO barrier");
    check(first.flushes == 0, "consumption does not flush implicitly");
    service.clear();
    jt::log::info(*second_log, "drain before final flush");
  }
  check(first.flushes == 1 && first.flushed_lines == 1,
        "shutdown flushes an already consumed unregistered logger");
  check(second.flushes == 2 && second.flushed_lines == 1,
        "shutdown flush follows remaining writes after explicit flush");
  check(first.destroyed == 0 && second.destroyed == 0,
        "shutdown flush does not require sink destruction");
}

void stop_race() {
  state s;
  std::shared_ptr<jt::log::logger> log;
  {
    jt::log::service service;
    log = make_logger(service, s, true);
    std::latch started(4);
    std::vector<std::jthread> producers;
    for (int p = 0; p < 4; ++p)
      producers.emplace_back([&, p] {
        started.count_down();
        started.wait();
        for (int i = 0; i < 500; ++i) jt::log::info(*log, "{}:{}", p, i);
      });
    started.wait();
    service.request_stop();
    producers.clear();
    service.request_stop();
  }
  const auto count = s.lines.size();
  jt::log::info(*log, "stopped");
  check(s.lines.size() == count, "stop closes submissions");
}

int main() {
  try {
    memory_and_buffer();
    buffer_self_append();
    buffer_boundaries();
    logger_ranges();
    synchronous();
    asynchronous();
    shutdown_flush();
    stop_race();
    std::println("behavior tests passed");
  } catch (const std::exception& e) {
    std::println("{}", e.what());
    return 1;
  }
}
