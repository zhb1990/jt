#include <lz4frame.h>

import std;
import jt;

namespace mem = jt::base;
namespace fs = std::filesystem;

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

void retention_and_expiry() {
  temporary_directory temp;
  const auto archive = temp.path.string();
  const auto old = temp.path / "keep_20000101.log.lz4";
  const auto other = temp.path / "other_20000101.log.lz4";
  const auto recent = temp.path / "keep_recent.log.lz4";
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

int main() {
  try {
    rotation_and_manifest();
    daily_rotation();
    retention_and_expiry();
    publish_failure_preserves_source();
    std::println("file tests passed");
  } catch (const std::exception& e) {
    std::println("{}", e.what());
    return 1;
  }
}
