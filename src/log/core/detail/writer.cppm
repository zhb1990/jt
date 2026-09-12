module jt.log.core:writer;

import std;
import jt.base.memory;
import jt.base.containers;
import jt.base.buffer;
import jt.detail.intrusive_mpsc_queue;
import jt.log.level;
import :fwd;
import :message;
import :archive;

namespace jt::log {

// Owns only log submission and dispatch. Archive outlives this backend.
class writer_backend {
 public:
  using logger_wptr = std::weak_ptr<logger>;

  explicit writer_backend(archive_worker& archive) : archive_(archive) {}
  ~writer_backend() noexcept;
  void start();
  void request_stop();
  void wait_stop();
  void flush(const logger_wptr& ptr);
  void log(const logger_wptr& ptr, std::uint32_t sid, level lv,
           base::buffer_1k& buf, const std::source_location& source);

 private:
  message* new_log_message();
  void delete_log_message(message* msg);
  void push_log_message(message* msg);
  void writer_do_message();
  void writer_run();

  archive_worker& archive_;
  std::thread writer_thread_{};
  std::mutex writer_mutex_{};
  std::condition_variable writer_cv_{};
  detail::intrusive_mpsc_queue<&message::next> writer_queue_{};
  bool writer_ready_{false};
  std::atomic_bool writer_stop_requested_{false};
  std::atomic<std::ptrdiff_t> writer_submission_counter_{0};
  base::allocator<message> message_allocator_{};
  // Accessed only by the writer thread; does not retain loggers or sinks.
  base::unordered_map<logger*, logger_wptr> dirty_loggers_;
};

}  // namespace jt::log
