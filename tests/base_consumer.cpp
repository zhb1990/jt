import std;
import jt.base.memory;
import jt.base.buffer;
import jt.base.containers;
import jt.base.concepts;

static_assert(jt::base::writable_buffer<jt::base::buffer_1k>);
static_assert(!jt::base::writable_buffer<int>);
static_assert(std::same_as<jt::base::vector<int>::allocator_type,
                           jt::base::allocator<int>>);
static_assert(
    std::same_as<jt::base::string::allocator_type, jt::base::allocator<char>>);

void check(bool condition) {
  if (!condition) throw std::runtime_error("aligned allocation regression");
}

struct polymorphic_base {
  virtual ~polymorphic_base() = default;
};

template <std::size_t Alignment>
struct alignas(Alignment) aligned_object : polymorphic_base {
  explicit aligned_object(bool fail = false) {
    check(reinterpret_cast<std::uintptr_t>(this) % Alignment == 0);
    if (fail) throw 42;
  }
  ~aligned_object() override { ++destroyed; }
  static inline int destroyed = 0;
  int value = 17;
};

template <std::size_t Alignment>
void check_alignment() {
  using object = aligned_object<Alignment>;
  const auto before = jt::base::allocated_memory();
  const auto destroyed = object::destroyed;
  {
    jt::base::allocator<object> allocator;
    for (const auto count : {1u, 3u, 17u}) {
      auto* ptr = allocator.allocate(count);
      check(reinterpret_cast<std::uintptr_t>(ptr) % Alignment == 0);
      check(jt::base::allocated_size(ptr) >= sizeof(object) * count);
      allocator.deallocate(ptr, count);
    }
    jt::base::vector<object> values(3);
    check(values[2].value == 17);
    auto single = jt::base::make_unique<object>();
    auto dynamic = jt::base::make_dynamic_unique<polymorphic_base, object>();
    check(single->value == 17);
    check(dynamic_cast<object*>(dynamic.get())->value == 17);
    for (const bool polymorphic : {false, true}) {
      bool caught = false;
      const auto prior = jt::base::allocated_memory();
      try {
        if (polymorphic) {
          auto ptr =
              jt::base::make_dynamic_unique<polymorphic_base, object>(true);
        } else {
          auto ptr = jt::base::make_unique<object>(true);
        }
      } catch (int error) {
        caught = error == 42;
      }
      check(caught && jt::base::allocated_memory() == prior);
    }
  }
  check(object::destroyed == destroyed + 5);
  check(jt::base::allocated_memory() == before);
}

template <typename Source, typename Destination>
void check_buffer_copy() {
  for (const auto size : {0u, 100u, 3000u}) {
    Source source;
    const std::string payload(size, 'x');
    source.append(payload);
    source.read(size / 3);
    const auto verify = [&](const Destination& copy) {
      if (copy.capacity() < source.capacity() ||
          copy.prependable() != source.prependable() ||
          copy.readable() != source.readable() ||
          std::memcmp(copy.data(), source.data(), size) != 0 ||
          copy.data() == source.data()) {
        throw std::runtime_error("cross-capacity buffer copy regression");
      }
    };
    Destination copied(source);
    verify(copied);
    Destination assigned;
    assigned.append("old contents");
    assigned.read(2);
    assigned = source;
    verify(assigned);
  }
}

int main() {
  check_buffer_copy<jt::base::buffer_1k, jt::base::buffer_2k>();
  check_buffer_copy<jt::base::buffer_2k, jt::base::buffer_1k>();
  check_alignment<64>();
  check_alignment<256>();
  check_alignment<4096>();
  auto* raw = jt::base::allocate(123, 256);
  check(reinterpret_cast<std::uintptr_t>(raw) % 256 == 0);
  check(jt::base::allocated_size(raw) >= 123);
  jt::base::deallocate(raw);
  jt::base::vector<int> vector{1, 2};
  jt::base::string string = "base";
  jt::base::wstring wide = L"base";
  jt::base::deque<int> deque{3};
  jt::base::unordered_map<int, int> map{{1, 2}};
  jt::base::unordered_multimap<int, int> multimap{{1, 2}, {1, 3}};
  jt::base::buffer_1k buffer;
  std::format_to(std::back_inserter(buffer), "{} {}", string, vector.size());
  auto number = jt::base::make_unique<int>(42);
  return std::string_view(buffer) == "base 2" && *number == 42 &&
                 deque.front() == 3 && map.at(1) == 2 &&
                 multimap.count(1) == 2 && wide.size() == 4
             ? 0
             : 1;
}
