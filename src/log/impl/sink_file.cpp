module;

#include <simdjson.h>

// module jt:log.sink.file;
module jt;

import std;

namespace jt::log {

class sink_file_imp {
 public:
  explicit sink_file_imp(service& s, const sink_file_config& config)  // NOLINT
      : service_(s),
        max_size_(config.max_size),
        daily_rotation_(config.daily_rotation),
        keep_days_(config.keep_days) {
    name_ = config.name;
    directory_ = config.directory;
    lz4_directory_ = config.lz4_directory;

    detail::buffer_1k temp;
    std::format_to(std::back_inserter(temp), "manifest_{}.json", name_);

    std::u8string_view u8strv(
        reinterpret_cast<const char8_t*>(directory_.c_str()),
        directory_.size());
    manifest_path_ = u8strv;
    std::error_code ec;
    create_directories(manifest_path_, ec);
    u8strv = {reinterpret_cast<const char8_t*>(temp.begin_read()),
              temp.readable()};
    manifest_path_ /= u8strv;
    load_manifest();

    u8strv = {reinterpret_cast<const char8_t*>(lz4_directory_.c_str()),
              lz4_directory_.size()};
    const std::filesystem::path lz4_directory = u8strv;
    create_directories(lz4_directory, ec);
  }

  void write(const sink::time_point& point, const detail::buffer_1k& buf) {
    if (tomorrow_ < point) {
      const auto old_day = manifest_.day;
      tomorrow_ =
          std::chrono::floor<std::chrono::days>(point) + std::chrono::days(1);
      if (daily_rotation_ || manifest_.day == 0) {
        rotate();
      }

      if (keep_days_ > 0 && old_day > 0) {
        service_.clear_lz4(name_, lz4_directory_, keep_days_);
      }
    }

    if (file_size_ >= max_size_) {
      rotate();
    }

    if (!file_.is_open()) {
      file_open();
      if (!file_.is_open()) {
        return;
      }
    }

    file_.write(reinterpret_cast<const char*>(buf.begin_read()),
                static_cast<std::streamsize>(buf.readable()));
    file_size_ += buf.readable();
  }

  void flush_unlock() {  // NOLINT(*-convert-member-functions-to-static)
    if (file_.is_open()) {
      file_.flush();
    }
  }

 private:
  void load_manifest() {
    std::ifstream manifest(manifest_path_, std::ios_base::binary);
    if (!manifest.is_open()) {
      return;
    }

    const detail::string data((std::istreambuf_iterator(manifest)),
                              std::istreambuf_iterator<char>());
    const simdjson::padded_string padded_data(data.c_str(), data.size());
    simdjson::ondemand::document doc;
    // ReSharper disable once CppTooWideScopeInitStatement
    simdjson::ondemand::parser parser;
    if (parser.iterate(padded_data).get(doc)) {
      return;
    }

    std::uint32_t day{0};
    if (doc["day"].get(day)) {
      return;
    }

    std::uint32_t seq{0};
    if (doc["seq"].get(seq)) {
      return;
    }

    manifest_.day = day;
    manifest_.seq = seq;
    const int year = static_cast<int>(day / 10000);
    const int month = static_cast<int>(day % 10000) / 100;
    const int m_day = static_cast<int>(day % 100);
    tomorrow_ = std::chrono::year_month_day(std::chrono::year(year),
                                            std::chrono::month(month),
                                            std::chrono::day(m_day));
    tomorrow_ += std::chrono::days{1};
    return file_open();
  }

  void save_manifest() {
    std::ofstream file(manifest_path_, std::ios::binary);
    detail::buffer_1k temp;
    std::format_to(std::back_inserter(temp), R"({{ "day":{}, "seq":{} }})",
                   manifest_.day, manifest_.seq);
    file.write(reinterpret_cast<const char*>(temp.begin_read()),
               static_cast<std::streamsize>(temp.readable()));
  }

  void rotate() {
    if (file_.is_open()) {
      file_.close();
      file_size_ = 0;
      service_.post_lz4(file_name_, lz4_directory_);
    }

    const std::chrono::year_month_day today{tomorrow_ - std::chrono::days{1}};
    const std::int32_t day = int{today.year()} * 10000 +
                             unsigned{today.month()} * 100 +
                             unsigned{today.day()};
    if (manifest_.day < day) {  // NOLINT(*-branch-clone)
      manifest_.day = day;
      manifest_.seq = 0;
      save_manifest();
    } else {
      ++manifest_.seq;
      save_manifest();
    }
  }

  void file_open() {
    detail::buffer_1k temp;
    if (manifest_.seq == 0) {  // NOLINT(*-branch-clone)
      std::format_to(std::back_inserter(temp), "{}_{}.log", name_,
                     manifest_.day);
    } else {
      std::format_to(std::back_inserter(temp), "{}_{}_{:04d}.log", name_,
                     manifest_.day, manifest_.seq);
    }

    std::u8string_view u8strv(
        reinterpret_cast<const char8_t*>(directory_.c_str()),
        directory_.size());
    file_name_ = u8strv;
    u8strv = {reinterpret_cast<const char8_t*>(temp.begin_read()),
              temp.readable()};
    file_name_ /= u8strv;
    if (std::filesystem::exists(file_name_)) {
      file_size_ = std::filesystem::file_size(file_name_);
    }
    file_.open(file_name_, std::ios::binary | std::ios::app);
  }

  service& service_;
  detail::string name_{};
  detail::string directory_{};
  detail::string lz4_directory_{};
  std::size_t max_size_;
  bool daily_rotation_;
  std::uint32_t keep_days_;

  struct manifest {
    // 年月日，例如20260102
    std::uint32_t day{0};
    // 日志序号
    std::uint32_t seq{0};
  };
  manifest manifest_{};
  std::filesystem::path manifest_path_;
  std::ofstream file_;
  std::filesystem::path file_name_;
  std::size_t file_size_{0};
  std::chrono::sys_days tomorrow_{};
};

sink_file::sink_file(service& s, const sink_file_config& config)  // NOLINT
    : impl_(detail::make_unique<sink_file_imp>(s, config)) {}

sink_file::~sink_file() noexcept = default;

void sink_file::write(level, const time_point& point,
                      const detail::buffer_1k& buf, std::size_t, std::size_t) {
  return impl_->write(point, buf);
}

void sink_file::flush_unlock() { return impl_->flush_unlock(); }

}  // namespace jt::log