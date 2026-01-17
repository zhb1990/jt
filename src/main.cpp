import jt;
import std;

struct test_node {
  std::int64_t value{0};
  test_node* next{nullptr};
};

int main(int argc, char** argv) {
  std::println("mem {}", jt::detail::allocated_memory());
  {
    std::array a1{
        jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_stdout>()};
    jt::log::service service;
    service.start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    const auto log1 = service.create_logger(std::move(a1), "test", true);
    jt::log::info(log1, "mem {}", jt::detail::allocated_memory());

    jt::detail::base_memory_buffer<1> b1;
    b1.shrink();
    jt::log::info(log1, "readable {}", b1.readable());

    {
      jt::detail::base_memory_buffer<1> buffer;
      buffer.append("hello");
      std::string_view strv(buffer);
      jt::log::info(log1, "{}", strv);
      std::format_to(std::back_inserter(buffer), " {}", "world");
      std::string_view strv1(buffer);
      jt::log::info(log1, "{}", strv1);
      jt::log::info(log1, "mem {}", jt::detail::allocated_memory());
      const jt::detail::read_buffer rb(buffer);
      jt::log::info(log1, "{}", std::string_view(rb));
    }
    jt::log::info(log1, "mem {}", jt::detail::allocated_memory());

    {
      void* ptr = jt::detail::allocate(100);
      jt::log::info(log1, "mem {}", jt::detail::allocated_memory());
      jt::log::info(log1, "ptr size {}", jt::detail::allocated_size(ptr));
      jt::detail::deallocate(ptr, 100);
      jt::log::info(log1, "mem {}", jt::detail::allocated_memory());
    }

    jt::log::warn(log1, "使用的内存 {}", jt::detail::allocated_memory());
    service.stop();
    std::println("mem {}", jt::detail::allocated_memory());
  }
  std::println("mem {}", jt::detail::allocated_memory());

  return 0;
}