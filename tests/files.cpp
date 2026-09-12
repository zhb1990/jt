#include <lz4frame.h>

import std;
import jt;

namespace mem = jt::base;
namespace fs = std::filesystem;

#ifdef JT_TEST_FILE_LIMITS
bool limit_file_size(unsigned long long bytes);
void restore_file_size_limit() noexcept;
#endif

void check(bool condition, std::string_view message) {
  if (!condition) throw std::runtime_error(std::string(message));
}

struct temporary_directory {
  fs::path path =
      fs::temp_directory_path() /
      std::format("jt-tests-{}",
                  std::chrono::steady_clock::now().time_since_epoch().count());
  temporary_directory() { fs::create_directories(path); }
  ~temporary_directory() {
    std::error_code ec;
    fs::remove_all(path, ec);
  }
};

std::string read(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

std::string decompress(const fs::path& path) {
  const auto input = read(path);
  LZ4F_decompressionContext_t context = nullptr;
  check(!LZ4F_isError(LZ4F_createDecompressionContext(&context, LZ4F_VERSION)),
        "decompression context");
  struct cleanup {
    LZ4F_decompressionContext_t context;
    ~cleanup() { LZ4F_freeDecompressionContext(context); }
  } guard{context};
  std::string output;
  std::size_t offset = 0;
  std::size_t remaining = 1;
  while (offset < input.size() && remaining != 0) {
    char buffer[4096];
    std::size_t source_size = input.size() - offset;
    std::size_t destination_size = sizeof(buffer);
    remaining = LZ4F_decompress(context, buffer, &destination_size,
                                input.data() + offset, &source_size, nullptr);
    check(!LZ4F_isError(remaining), "valid LZ4 frame");
    check(source_size != 0 || destination_size != 0, "decompression progress");
    output.append(buffer, destination_size);
    offset += source_size;
  }
  check(remaining == 0, "complete LZ4 frame");
  return output;
}

void write_record(jt::log::sink& sink, std::string_view payload,
                  std::chrono::system_clock::time_point time =
                      std::chrono::system_clock::now()) {
  sink.consume(
      {.lv = jt::log::level::info, .timestamp = time, .payload = payload});
  sink.flush();
}

void shutdown_flush_retained_logger() {
  temporary_directory temp;
  const auto directory = temp.path.string();
  const auto archive = (temp.path / "archive").string();
  std::shared_ptr<jt::log::logger> log;
  {
    jt::log::service service;
    jt::log::sink_file_config config{
        .name = "shutdown", .directory = directory, .lz4_directory = archive};
    auto sink = mem::make_dynamic_unique<jt::log::sink, jt::log::sink_file>(
        service, config);
    log = service.create_logger(std::array{std::move(sink)}, "shutdown", true);
    jt::log::info(*log, "short shutdown record");
    service.clear();
  }
  std::size_t files = 0;
  for (const auto& entry : fs::directory_iterator(temp.path)) {
    if (entry.path().extension() == ".log") {
      ++files;
      check(
          read(entry.path()).find("short shutdown record") != std::string::npos,
          "shutdown makes buffered file contents visible with retained logger");
    }
  }
  check(files == 1, "shutdown created one log file");
}

void rotation_and_manifest() {
  temporary_directory temp;
  const auto directory = temp.path.string();
  const auto archive = (temp.path / "archive").string();
  jt::log::sink_file_config config{.name = "rotation",
                                   .directory = directory,
                                   .lz4_directory = archive,
                                   .max_size = 1,
                                   .keep_days = 1};
  {
    jt::log::service service;
    jt::log::sink_file sink(service, config);
    write_record(sink, "first-record");
    write_record(sink, "second-record");
  }
  check(read(temp.path / "manifest_rotation.json").find("\"seq\":1") !=
            std::string::npos,
        "manifest sequence after size rotation");
  std::size_t archives = 0;
  for (const auto& entry : fs::directory_iterator(archive)) {
    check(!entry.path().string().ends_with(".tmp"), "no unfinished archive");
    if (entry.path().extension() == ".lz4") {
      ++archives;
      check(decompress(entry.path()).find("first-record") != std::string::npos,
            "archive contents");
    }
  }
  check(archives == 1, "archive drained before service destruction returns");
  config.max_size = 1024 * 1024;
  {
    jt::log::service service;
    jt::log::sink_file sink(service, config);
    write_record(sink, "after-restart");
  }
  bool appended = false;
  for (const auto& entry : fs::directory_iterator(temp.path)) {
    if (entry.path().extension() == ".log") {
      const auto content = read(entry.path());
      appended |= content.find("second-record") != std::string::npos &&
                  content.find("after-restart") != std::string::npos;
    }
  }
  check(appended, "manifest restores append target");
}

void daily_rotation() {
  temporary_directory temp;
  const auto directory = temp.path.string();
  const auto archive = (temp.path / "archive").string();
  {
    jt::log::service service;
    jt::log::sink_file sink(service, {.name = "daily",
                                      .directory = directory,
                                      .lz4_directory = archive,
                                      .max_size = 1024 * 1024,
                                      .keep_days = 0});
    const auto now = std::chrono::system_clock::now();
    write_record(sink, "day-one", now);
    write_record(sink, "day-three", now + std::chrono::hours(48));
  }
  std::size_t count = 0;
  for (const auto& entry : fs::directory_iterator(archive)) {
    if (entry.path().extension() == ".lz4") {
      ++count;
      check(decompress(entry.path()).find("day-one") != std::string::npos,
            "daily archive contents");
    }
  }
  check(count == 1, "daily rotation");
}

void midnight_rotation() {
  using namespace std::chrono;
  for (const bool restart : {false, true}) {
    temporary_directory temp;
    const auto directory = temp.path.string();
    const auto archive = (temp.path / "archive").string();
    jt::log::sink_file_config config{.name = "midnight",
                                     .directory = directory,
                                     .lz4_directory = archive,
                                     .max_size = 1024 * 1024,
                                     .keep_days = 0};
    const auto midnight =
        current_zone()->to_sys(local_days{year{2026} / January / 15});
    {
      jt::log::service service;
      auto sink = mem::make_unique<jt::log::sink_file>(service, config);
      write_record(*sink, "before-midnight", midnight - seconds(1));
      if (restart) {
        sink.reset();
        sink = mem::make_unique<jt::log::sink_file>(service, config);
      }
      write_record(*sink, "at-midnight", midnight);
      const auto current = read(temp.path / "midnight_20260115.log");
      check(current.find("at-midnight") != std::string::npos &&
                current.find("before-midnight") == std::string::npos,
            "midnight record immediately enters new file");
      write_record(*sink, "after-midnight", midnight + seconds(1));
    }
    const auto old =
        decompress(temp.path / "archive" / "midnight_20260114.log.lz4");
    check(old.find("before-midnight") != std::string::npos &&
              old.find("at-midnight") == std::string::npos,
          "old archive excludes midnight record");
    const auto current = read(temp.path / "midnight_20260115.log");
    check(current.find("after-midnight") != std::string::npos,
          "same day continues in new file");
  }
}

void retention_and_expiry() {
  temporary_directory temp;
  const auto archive = temp.path.string();
  const auto old = temp.path / "keep_20000101.log.lz4";
  const auto other = temp.path / "other_20000101.log.lz4";
  const auto recent = temp.path / "keep_20000102.log.lz4";
  for (const auto& file : {old, other, recent})
    std::ofstream(file) << "fixture";
  const auto past = fs::file_time_type::clock::now() - std::chrono::hours(72);
  fs::last_write_time(old, past);
  fs::last_write_time(other, past);
  std::optional<jt::log::service::lz4_client> client;
  {
    jt::log::service service;
    client.emplace(service.make_lz4_client());
    client->clear("keep", archive, 1);
  }
  check(!fs::exists(old) && fs::exists(other) && fs::exists(recent),
        "retention selection");
  const auto source = temp.path / "expired.log";
  std::ofstream(source) << "preserve";
  client->post(source, archive);
  client->clear("other", archive, 1);
  check(fs::exists(source) && fs::exists(other),
        "expired archive client is inert");
}

void retention_exact_names() {
  for (const std::string_view name : {"app", "", "app_worker"}) {
    temporary_directory temp;
    const auto past = fs::file_time_type::clock::now() - std::chrono::hours(72);
    const auto prefix = std::string(name) + "_";
    const auto create_old = [&](const std::string& filename) {
      const auto path = temp.path / filename;
      std::ofstream(path) << "fixture";
      fs::last_write_time(path, past);
    };
    for (const auto suffix : {"20000101.log.lz4", "20000101_0001.log.lz4",
                              "20000101_10000.log.lz4"}) {
      create_old(prefix + suffix);
    }
    mem::vector<std::string> preserved;
    for (const auto suffix :
         {"2000010.log.lz4", "200001011.log.lz4", "2000x101.log.lz4",
          "20000101_.log.lz4", "20000101_001.log.lz4", "20000101_000x.log.lz4",
          "20000101_0001_extra.log.lz4", "20000101.log.lz4.tmp",
          "20000101.log"}) {
      preserved.push_back(prefix + suffix);
    }
    for (const auto other : {"application", "app_worker", "_worker", "app"}) {
      if (name != other) {
        preserved.push_back(std::string(other) + "_20000101.log.lz4");
      }
    }
    for (const auto& filename : preserved) create_old(filename);
    const auto recent = temp.path / (prefix + "20000102.log.lz4");
    std::ofstream(recent) << "recent";
    const auto directory = temp.path / (prefix + "20000103.log.lz4");
    fs::create_directory(directory);
    fs::last_write_time(directory, past);
    {
      jt::log::service service;
      service.make_lz4_client().clear(name, temp.path.string(), 1);
    }
    for (const auto suffix : {"20000101.log.lz4", "20000101_0001.log.lz4",
                              "20000101_10000.log.lz4"}) {
      check(!fs::exists(temp.path / (prefix + suffix)),
            "matching expired archive removed");
    }
    for (const auto& filename : preserved) {
      check(fs::exists(temp.path / filename),
            std::format("cleanup for '{}' preserves '{}'", name, filename));
    }
    check(fs::exists(recent), "matching recent archive preserved");
    check(fs::is_directory(directory), "matching directory preserved");
  }
}

void archive_name_collision() {
  temporary_directory temp;
  const auto archive = temp.path / "archive";
  fs::create_directories(archive);
  const auto first = temp.path / "first" / "same.log";
  const auto second = temp.path / "second" / "same.log";
  fs::create_directories(first.parent_path());
  fs::create_directories(second.parent_path());
  std::ofstream(first) << "first contents";
  const auto post = [&](const fs::path& source) {
    jt::log::service service;
    service.make_lz4_client().post(source, archive.string());
  };
  post(first);
  const auto final = archive / "same.log.lz4";
  check(!fs::exists(first), "successful publication removes source");
  check(decompress(final) == "first contents", "first archive contents");
  const auto original = read(final);
  // Different source directory, then reuse the original source filename.
  for (const auto& source : {second, first}) {
    std::ofstream(source) << "later contents";
    post(source);
    check(read(final) == original, "existing archive is never overwritten");
    check(read(source) == "later contents", "collision preserves source");
    check(!fs::exists(archive / "same.log.lz4.tmp"),
          "collision cleans up owned temporary file");
  }
}

void archive_temporary_collision() {
  temporary_directory temp;
  const auto source = temp.path / "source.log";
  const auto temporary = temp.path / "source.log.lz4.tmp";
  std::ofstream(source) << "source contents";
  std::ofstream(temporary) << "existing temporary contents";
  {
    jt::log::service service;
    service.make_lz4_client().post(source, temp.path.string());
  }
  check(read(source) == "source contents",
        "temporary collision preserves source");
  check(read(temporary) == "existing temporary contents",
        "temporary collision preserves existing file");
  check(!fs::exists(temp.path / "source.log.lz4"),
        "temporary collision does not publish");
}

void publish_failure_preserves_source() {
  temporary_directory temp;
  const auto archive = temp.path / "archive";
  fs::create_directories(archive / "source.log.lz4");
  std::ofstream(archive / "source.log.lz4" / "occupied") << "block rename";
  const auto source = temp.path / "source.log";
  std::ofstream(source) << "must survive";
  {
    jt::log::service service;
    service.make_lz4_client().post(source, archive.string());
  }
  check(read(source) == "must survive",
        "failed archive publication preserves source");
}

void retention_limit() {
  temporary_directory temp;
  jt::log::service service;
  for (const auto days :
       {0u, 1095u, 1096u, std::numeric_limits<std::uint32_t>::max()}) {
    const auto path = temp.path / std::to_string(days);
    const auto directory = path.string();
    const auto archive = (path / "archive").string();
    bool rejected = false;
    try {
      jt::log::sink_file sink(service, {.name = "limit",
                                        .directory = directory,
                                        .lz4_directory = archive,
                                        .keep_days = days});
    } catch (const std::invalid_argument&) {
      rejected = true;
    }
    check(rejected == (days > 1095), "retention construction boundary");
    if (rejected) check(!fs::exists(path), "reject before filesystem changes");
  }
  const auto fresh = temp.path / "limit_20260912.log.lz4";
  std::ofstream(fresh) << "preserve";
  bool rejected = false;
  try {
    service.make_lz4_client().clear("limit", temp.path.string(),
                                    std::numeric_limits<std::uint32_t>::max());
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  check(rejected && read(fresh) == "preserve",
        "direct cleanup rejects overflow");
}

void file_error_recovery() {
#ifdef JT_TEST_FILE_LIMITS
  struct payload_formatter : jt::log::formatter {
    void format(const jt::log::log_record_view& record, mem::buffer_1k& output,
                std::size_t& start, std::size_t& stop) override {
      output.append(record.payload);
      start = stop = 0;
    }
  };
  for (const auto size : {5u, 8192u}) {
    temporary_directory temp;
    const auto directory = temp.path.string();
    const auto archive = (temp.path / "archive").string();
    const auto point = std::chrono::system_clock::now();
    {
      jt::log::service service;
      jt::log::sink_file sink(service, {.name = "recover",
                                        .directory = directory,
                                        .lz4_directory = archive,
                                        .max_size = 64});
      sink.set_formatter(
          mem::make_dynamic_unique<jt::log::formatter, payload_formatter>());
      write_record(sink, "seed\n", point);
      fs::path source;
      for (const auto& entry : fs::directory_iterator(temp.path)) {
        if (entry.path().extension() == ".log") source = entry.path();
      }
      check(!source.empty(), "recovery source exists");
      bool failed = false;
      {
        struct restore_limit {
          ~restore_limit() { restore_file_size_limit(); }
        } restore;
        check(limit_file_size(fs::file_size(source) + 1), "set file limit");
        try {
          write_record(sink, std::string(size, 'x'), point);
        } catch (const std::ios_base::failure&) {
          failed = true;
        }
      }
      check(failed, "write or flush failure reaches direct sink caller");
      write_record(sink, "recovered\n", point);
      const auto contents = read(source);
      check(contents.find("recovered\n") != std::string::npos &&
                contents.find("recovered\n") == contents.rfind("recovered\n"),
            "later record written once after I/O recovery");
      check(read(temp.path / "manifest_recover.json").find("\"seq\":0") !=
                std::string::npos,
            "failed bytes do not trigger premature rotation");
      write_record(sink, std::string(70, 'r'), point);
      write_record(sink, "after rotation\n", point);
    }
    std::size_t archives = 0;
    for (const auto& entry : fs::directory_iterator(archive)) {
      if (entry.path().extension() != ".lz4") continue;
      ++archives;
      check(decompress(entry.path()).find("recovered\n") != std::string::npos,
            "rotation archives successfully recovered contents");
    }
    check(archives == 1, "rotation continues after recovery");
  }
#endif
}

int main() {
  try {
    retention_limit();
    file_error_recovery();
    shutdown_flush_retained_logger();
    rotation_and_manifest();
    daily_rotation();
    midnight_rotation();
    retention_and_expiry();
    retention_exact_names();
    archive_name_collision();
    archive_temporary_collision();
    publish_failure_preserves_source();
    std::println("file tests passed");
  } catch (const std::exception& e) {
    std::println("{}", e.what());
    return 1;
  }
}
