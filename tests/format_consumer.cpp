import std;
import jt;

#include "format_value.h"

void format_other(jt::base::buffer_1k& output);
void log_other(jt::log::logger& log);

namespace {

void check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct captured {
  std::vector<std::string> lines;
  std::uint_least32_t source_line{};
};

struct payload_formatter final : jt::log::formatter {
  explicit payload_formatter(captured& values) : values(values) {}
  void format(const jt::log::log_record_view& record,
              jt::base::buffer_1k& output, std::size_t& start,
              std::size_t& stop) override {
    output.append(record.payload);
    values.source_line = record.source.line();
    start = stop = 0;
  }
  captured& values;
};

struct capture_sink final : jt::log::sink {
  explicit capture_sink(captured& values) : values(values) {}
  void write(jt::log::level, const time_point&,
             const jt::base::buffer_1k& output, std::size_t,
             std::size_t) override {
    values.lines.emplace_back(std::string_view(output));
  }
  void flush_unlock() override {}
  captured& values;
};

}  // namespace

int main() {
  jt::base::buffer_1k output;
  jt::log::format_to(output, "{} {} ", format_value{"first"}, 42);
  format_other(output);
  check(std::string_view(output) == "first 42 other 0007 1.25 \"a\\nb\"",
        "formatting from two translation units");

  const std::string long_text(4096, 'x');
  output.clear();
  jt::log::format_to(output, "{}", long_text);
  check(std::string_view(output) == long_text, "formatting grows the buffer");
  bool threw = false;
  try {
    jt::log::vformat_to(output, "{", std::make_format_args());
  } catch (const std::format_error&) {
    threw = true;
  }
  check(threw, "raw formatting reports invalid format strings");

  jt::base::base_memory_buffer<128> date;
  jt::base::base_memory_buffer<32> zone;
  const auto timestamp = std::chrono::system_clock::now();
  jt::log::format_local_time(timestamp, date, zone);
  const auto local = std::chrono::zoned_time{
      std::chrono::current_zone(),
      std::chrono::floor<std::chrono::seconds>(timestamp)};
  const auto day =
      std::chrono::floor<std::chrono::days>(local.get_local_time());
  const std::chrono::year_month_day ymd{day};
  const std::chrono::hh_mm_ss time{local.get_local_time() - day};
  output.clear();
  jt::log::format_to(output, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
                     int(ymd.year()), unsigned(ymd.month()),
                     unsigned(ymd.day()), time.hours().count(),
                     time.minutes().count(), time.seconds().count());
  check(std::string_view(date) == std::string_view(output),
        "local date and time");
  const auto offset = local.get_info().offset.count();
  const auto magnitude = offset < 0 ? -offset : offset;
  output.clear();
  jt::log::format_to(output, "{}{:02}:{:02}", offset < 0 ? '-' : '+',
                     magnitude / 3600, magnitude / 60 % 60);
  check(std::string_view(zone) == std::string_view(output),
        "local zone offset");

  captured values;
  jt::log::service service;
  auto sink =
      jt::base::make_dynamic_unique<jt::log::sink, capture_sink>(values);
  sink->set_formatter(
      jt::base::make_dynamic_unique<jt::log::formatter, payload_formatter>(
          values));
  auto log =
      service.create_logger(std::array{std::move(sink)}, "format", false);
  const auto line = std::source_location::current().line() + 1;
  jt::log::info(*log, "{}", format_value{"first"});
  check(values.source_line == line, "caller source location");
  log_other(*log);
  jt::log::info(*log, "{}", long_text);
  check(values.lines == std::vector<std::string>{"first", "other 7",
                                                 "runtime 8", long_text},
        "typed, runtime and custom logging from two translation units");
  jt::log::print_stdout("format consumer {}\n", 1);
  jt::log::print_stderr("format consumer {}\n", format_value{"first"});

  captured decorated;
  auto default_sink =
      jt::base::make_dynamic_unique<jt::log::sink, capture_sink>(decorated);
  auto default_log = service.create_logger(std::array{std::move(default_sink)},
                                           "default", false);
  jt::log::info(*default_log, "{}", format_value{"default payload"});
  check(decorated.lines.size() == 1 &&
            decorated.lines.front().ends_with("default payload\n") &&
            decorated.lines.front().find("[info]") != std::string::npos,
        "default formatter dispatch across module boundary");
}
