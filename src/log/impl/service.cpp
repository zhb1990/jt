module;

module jt.log.core;

import std;
import :service_impl;
import jt.detail.string;
import jt.detail.buffer;
import jt.detail.memory;
import jt.log.level;

namespace jt::log {

service::service()
    : impl_(std::allocate_shared<service_impl>(
          detail::allocator<service_impl>{})) {}

service::~service() noexcept {
  impl_->request_stop();
  impl_->wait_stop();
}

// ReSharper disable once CppMemberFunctionMayBeConst
auto service::find(const std::string_view name) -> logger_sptr {
  return impl_->find(name);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void service::erase(const std::string_view name) { return impl_->erase(name); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::clear() { return impl_->clear(); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::request_stop() { return impl_->request_stop(); }

// ReSharper disable once CppMemberFunctionMayBeConst
auto service::get_default() -> logger_sptr { return impl_->get_default(); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::set_default(const logger_sptr& ptr) {
  return impl_->set_default(ptr);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void service::flush(const logger_wptr& ptr) { return impl_->flush(ptr); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::log(const logger_wptr& ptr, const std::uint32_t sid,
                  const level lv, detail::buffer_1k& buf,
                  const std::source_location& source) {
  return impl_->log(std::move(ptr), sid, lv, buf, source);
}

// ReSharper disable once CppMemberFunctionMayBeConst
auto service::create_logger(const std::string_view& name,  // NOLINT
                            const bool async, detail::vector<sink_ptr> sinks)
    -> logger_sptr {
  auto ptr = std::allocate_shared<logger>(detail::allocator<logger>{},
                                          logger::ctor_key{}, *this, name,
                                          std::move(sinks), async);
  impl_->register_logger(ptr);
  return ptr;
}

auto service::get_impl() -> std::shared_ptr<service_impl> { return impl_; }

auto service::make_lz4_client() const -> lz4_client {
  return lz4_client{std::static_pointer_cast<void>(impl_)};
}

void service::lz4_client::post(const std::filesystem::path& file_name,
                               const std::string_view lz4_directory) const {
  if (auto p = std::static_pointer_cast<service_impl>(impl_.lock())) {
    p->post_lz4(file_name, lz4_directory);
  }
}

void service::lz4_client::clear(const std::string_view name,
                                const std::string_view lz4_directory,
                                const std::uint32_t keep_days) const {
  if (auto p = std::static_pointer_cast<service_impl>(impl_.lock())) {
    p->clear_lz4(detail::string{name}, lz4_directory, keep_days);
  }
}

}  // namespace jt::log