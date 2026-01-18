// module jt:log.sink.file;
module jt;

import std;

namespace jt::log {

class sink_file_imp {
 public:
  sink_file_imp(const sink_file_config& config)
      : max_size_(config.max_size),
        daily_rotation_(config.daily_rotation),
        keep_days_(config.keep_days) {
    name_ = config.name;
    directory_ = config.directory;
    lz4_directory_ = config.lz4_directory;
    load_manifest();
  }

  void write(const sink::time_point& point, const detail::buffer_1k& buf) {
    if (tomorrow_ < point) {
      auto temp = std::chrono::floor<std::chrono::days>(point);
      tomorrow_ = temp + std::chrono::days(1);
      if (daily_rotation_ || manifest_.day == 0) {
        std::chrono::year_month_day today{temp};
        std::int32_t day = int{today.year()} * 10000 +
                           unsigned{today.month()} * 100 +
                           unsigned{today.day()};
        if (manifest_.day < day) {
            manifest_.day = day;
            manifest_.seq = 0;
            save_manifest();
        }

        rotate();
      }
    }

    if (file_size_ >= max_size_) {
      rotate();
    }

    if (!file_.is_open()) {
      return;
    }

    file_.write(reinterpret_cast<const char*>(buf.begin_read()),
                static_cast<std::streamsize>(buf.readable()));
    file_size_ += buf.readable();
  }

  void flush_unlock() {
    if (file_.is_open()) {
      file_.flush();
    }
  }

 private:
  void load_manifest() {}

  void save_manifest() {}

  void rotate() {
    if (file_.is_open()) {
      file_.close();
      file_size_ = 0;
      // todo: lz4 file_name_
    }
  }

  detail::string name_;
  detail::string directory_;
  detail::string lz4_directory_;
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
  std::ofstream file_;
  detail::string file_name_;
  std::size_t file_size_{0};
  std::chrono::sys_days tomorrow_{};
};

sink_file::sink_file(const sink_file_config& config)
    : impl_(detail::make_unique<sink_file_imp>(config)) {}

sink_file::~sink_file() noexcept = default;

void sink_file::write(level, const time_point& point,
                      const detail::buffer_1k& buf, std::size_t, std::size_t) {
  return impl_->write(point, buf);
}

void sink_file::flush_unlock() { return impl_->flush_unlock(); }

}  // namespace jt::log