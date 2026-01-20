module;

#include "../detail/config.h"

export module jt:log.service;

import std;
import :detail.memory;
import :detail.buffer;
import :detail.vector;
import :log.level;
import :log.sink;
import :log.fwd;

export namespace jt::log {

class service {
 public:
  using logger_sptr = std::shared_ptr<logger>;
  using logger_wptr = std::weak_ptr<logger>;
  using sink_ptr = detail::dynamic_unique_ptr<sink>;

  JT_API service();

  JT_API ~service() noexcept;

  JT_API logger_sptr find(std::string_view name);

  JT_API void erase(std::string_view name);

  JT_API void clear();

  JT_API void start();

  JT_API void stop();

  JT_API auto get_default() -> logger_sptr;

  JT_API void set_default(const logger_sptr& ptr);

  JT_API void flush(const logger_wptr& ptr);

  JT_API void log(const logger_wptr& ptr, std::uint32_t sid, level lv,
                  detail::buffer_1k& buf, const std::source_location& source);

  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, sink_ptr>
  auto create_logger(R&& range, const std::string_view& name, const bool async)
      -> logger_sptr {
    detail::vector<sink_ptr> sinks(
        std::make_move_iterator(std::ranges::begin(range)),
        std::make_move_iterator(std::ranges::end(range)));
    return create_logger(name, async, sinks);
  }

  JT_API auto create_logger(const std::string_view& name, bool async,
                            detail::vector<sink_ptr>& sinks) -> logger_sptr;

  void post_lz4(const std::filesystem::path& file_name,
                std::string_view lz4_directory);

 private:
  detail::unique_ptr<service_impl> impl_;
};

}  // namespace jt::log