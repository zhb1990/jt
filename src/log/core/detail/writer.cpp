module jt.log.core;

import std;
import jt.detail.os;
import jt.detail.cpu_pause;
import jt.base.buffer;
import jt.log.level;
import :writer;
import :archive;
import :message;

namespace jt::log {

constexpr std::ptrdiff_t thread_closed =
    std::numeric_limits<std::ptrdiff_t>::min() / 2;

writer_backend::~writer_backend() noexcept {
  request_stop();
  wait_stop();
}

void writer_backend::start() {
  writer_thread_ = std::thread{[this] { writer_run(); }};
}

void writer_backend::request_stop() {
  std::scoped_lock lock{writer_mutex_};
  writer_stop_requested_.store(true, std::memory_order::relaxed);
  writer_cv_.notify_one();
}

void writer_backend::wait_stop() {
  if (writer_thread_.joinable()) writer_thread_.join();
}

void writer_backend::flush(const logger_wptr& ptr) {
  message* msg = new_log_message();
  if (!msg) {
    return;
  }

  msg->target = ptr;
  msg->type = message_type::flush;
  return push_log_message(msg);
}

void writer_backend::log(const logger_wptr& ptr, const std::uint32_t sid,
                         const level lv, base::buffer_1k& buf,
                         const std::source_location& source) {
  message* msg = new_log_message();
  if (!msg) {
    return;
  }

  msg->target = ptr;
  msg->type = message_type::log;
  msg->payload = std::move(buf);
  msg->source = source;
  msg->lv = lv;
  msg->service_id = sid;
  msg->timestamp = std::chrono::system_clock::now();
  msg->thread_id = detail::tid();
  return push_log_message(msg);
}

message* writer_backend::new_log_message() {
  if (writer_stop_requested_.load(std::memory_order_relaxed)) {
    return nullptr;
  }

  message* msg = message_allocator_.allocate(1);
  message_allocator_.construct(msg);
  return msg;
}

void writer_backend::delete_log_message(message* msg) {
  message_allocator_.destroy(msg);
  message_allocator_.deallocate(msg, 1);
}

void writer_backend::push_log_message(message* msg) {
  std::ptrdiff_t n =
      writer_submission_counter_.fetch_add(1, std::memory_order::relaxed);
  if (n < 0) {
    delete_log_message(msg);
    writer_submission_counter_.fetch_sub(1, std::memory_order::relaxed);
    return;
  }

  if (writer_queue_.push_back(msg)) {
    std::scoped_lock lock{writer_mutex_};
    writer_ready_ = true;
    writer_cv_.notify_one();
  }
  writer_submission_counter_.fetch_sub(1, std::memory_order::release);
}

inline void writer_backend::writer_do_message() {
  message* msg = nullptr;
  bool is_empty = false;
  std::tie(msg, is_empty) = writer_queue_.pop_front();
  while (!is_empty) {
    // ReSharper disable once CppDFAEndlessLoop
    if (msg) {
      if (const auto ptr = msg->target.lock()) {
        if (msg->type == message_type::log) {
          ptr->backend_log(msg->record());
        } else {
          ptr->backend_flush();
        }
      }

      delete_log_message(msg);
    } else {
      detail::cpu_pause();
    }

    std::tie(msg, is_empty) = writer_queue_.pop_front();
  }
}

void writer_backend::writer_run() {
  while (true) {
    writer_do_message();

    std::unique_lock lock{writer_mutex_};
    writer_cv_.wait_for(lock, std::chrono::seconds(2), [this] {
      return writer_ready_ || writer_stop_requested_;
    });
    const bool stop_requested = writer_stop_requested_;
    writer_ready_ = false;
    lock.unlock();

    if (stop_requested) {
      std::ptrdiff_t expected = 0;
      while (!writer_submission_counter_.compare_exchange_weak(
          expected, thread_closed, std::memory_order::acquire,
          std::memory_order::relaxed)) {
        detail::cpu_pause();
        expected = 0;
      }

      writer_do_message();

      // No more file rotations can be submitted by this writer.
      archive_.request_stop();
      break;
    }
  }
}

}  // namespace jt::log
