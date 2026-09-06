module jt.log.core:service_impl;

import std;
import jt.base.buffer;
import jt.base.containers;
import jt.log.level;
import :fwd;
import :writer;
import :archive;

namespace jt::log {

// Registry and lifetime coordinator. Workers never own the service or loggers.
class service_impl {
 public:
  using logger_sptr = std::shared_ptr<logger>;
  using logger_wptr = std::weak_ptr<logger>;

  service_impl();
  ~service_impl() noexcept;
  void register_logger(logger_sptr& ptr);
  logger_sptr find(std::string_view name);
  void erase(std::string_view name);
  void clear();
  void request_stop();
  void wait_stop();
  auto get_default() -> logger_sptr;
  void set_default(const logger_sptr& ptr);

  void flush(const logger_wptr& ptr) { writer_.flush(ptr); }
  void log(const logger_wptr& ptr, std::uint32_t sid, level lv,
           base::buffer_1k& buf, const std::source_location& source) {
    writer_.log(ptr, sid, lv, buf, source);
  }
  void post_lz4(const std::filesystem::path& file_name,
                std::string_view directory) {
    archive_.post_lz4(file_name, directory);
  }
  void clear_lz4(const base::string& name, std::string_view directory,
                 std::uint32_t keep_days) {
    archive_.clear_lz4(name, directory, keep_days);
  }

 private:
  std::mutex loggers_mutex_{};
  base::unordered_map<std::string_view, logger_sptr> loggers_{};
  std::atomic<logger_sptr> default_logger_{};
  // Destruction is writer -> archive -> registry. Draining retains logger
  // targets.
  archive_worker archive_{};
  writer_backend writer_{archive_};
};

}  // namespace jt::log
