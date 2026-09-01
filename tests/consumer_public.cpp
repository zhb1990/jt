import jt;
import std;

int main() {
  jt::log::service service;
  std::array sinks{
      jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_stdout>()};
  const auto log_ptr =
      service.create_logger(std::move(sinks), "consumer", false);
  auto& log = *log_ptr;
  jt::log::info(log, "consumer ok {}", jt::detail::allocated_memory());
  jt::detail::base_memory_buffer<1> buffer;
  buffer.append("hello");
  jt::log::info(log, "{}", std::string_view(buffer));
  service.request_stop();
  return 0;
}
