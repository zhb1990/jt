import jt;
import std;

struct test_node {
  std::int64_t value{0};
  test_node* next{nullptr};
};

int main(int argc, char** argv) {
  jt::detail::base_memory_buffer<1> b1;
  b1.shrink();
  std::println("readable {}", b1.readable());

  {
    jt::detail::base_memory_buffer<1> buffer;
    buffer.append("hello");
    std::string_view strv(buffer);
    std::println("{}", strv);
    std::format_to(std::back_inserter(buffer), " {}", "world");
    std::string_view strv1(buffer);
    std::println("{}", strv1);
    std::println("mem {}", jt::detail::allocated_memory());
    const jt::detail::read_buffer rb(buffer);
    std::println("{}", std::string_view(rb));
  }
  std::println("mem {}", jt::detail::allocated_memory());

  {
    void* ptr = jt::detail::allocate(100);
    std::println("mem {}", jt::detail::allocated_memory());
    std::println("ptr size {}", jt::detail::allocated_size(ptr));
    jt::detail::deallocate(ptr, 100);
    std::println("mem {}", jt::detail::allocated_memory());
  }

  return 0;
}