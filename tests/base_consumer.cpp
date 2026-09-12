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
    check(static_cast<object*>(dynamic.get())->value == 17);
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

struct owner_state {
  int destroyed{0};
  const void* allocation{nullptr};
};

struct owner_base {
  virtual ~owner_base() = default;
  virtual int value() const = 0;
};

struct leading_base {
  virtual ~leading_base() = default;
  std::uint64_t padding[2]{};
};

struct alignas(256) owner_object : leading_base, owner_base {
  owner_object(owner_state& state, int value, bool fail = false)
      : state(state), number(value) {
    state.allocation = this;
    check(reinterpret_cast<std::uintptr_t>(this) % 256 == 0);
    if (fail) throw 42;
  }
  ~owner_object() override { ++state.destroyed; }
  int value() const override { return number; }
  owner_state& state;
  int number;
};

struct virtual_owner_object : leading_base, virtual owner_base {
  explicit virtual_owner_object(owner_state& state) : state(state) {
    state.allocation = this;
  }
  ~virtual_owner_object() override { ++state.destroyed; }
  int value() const override { return 9; }
  owner_state& state;
};

using owner = jt::base::dynamic_unique_ptr<owner_base>;

template <typename T>
concept raw_resettable =
    requires(T& p, typename T::element_type* raw) { p.reset(raw); };
template <typename T>
concept releasable = requires(T& p) { p.release(); };
template <typename T>
concept exposes_deleter = requires(T& p) { p.get_deleter(); };

static_assert(!std::is_copy_constructible_v<owner>);
static_assert(!std::is_copy_assignable_v<owner>);
static_assert(!std::is_constructible_v<owner, owner_base*>);
static_assert(!std::is_constructible_v<owner, owner_base*, void*>);
static_assert(!raw_resettable<owner> && !releasable<owner> &&
              !exposes_deleter<owner>);
static_assert(std::is_nothrow_move_constructible_v<owner>);
static_assert(std::is_nothrow_move_assignable_v<owner>);
static_assert(std::is_nothrow_destructible_v<owner>);
static_assert(std::is_nothrow_swappable_v<owner>);
static_assert(noexcept(std::declval<owner&>().reset()));

struct owner_parent : owner_base {
  owner_parent(owner_state& state, owner child)
      : state(state), child(std::move(child)) {}
  ~owner_parent() override { ++state.destroyed; }
  int value() const override { return 0; }
  owner_state& state;
  owner child;
};

void check_ownership() {
  const auto before = jt::base::allocated_memory();
  owner_state a, b, c, v, constant, parent, child, failed;
  {
    owner empty;
    owner null = nullptr;
    check(!empty && empty.get() == nullptr && empty == nullptr &&
          nullptr == null && !(empty != nullptr));
    empty.reset();
    empty.reset(nullptr);
    empty = nullptr;

    auto first = jt::base::make_dynamic_unique<owner_base, owner_object>(a, 1);
    auto second = jt::base::make_dynamic_unique<owner_base, owner_object>(b, 2);
    check(static_cast<const void*>(first.get()) != a.allocation);
    check(first != nullptr && nullptr != first && first->value() == 1 &&
          (*first).value() == 1);
    auto* original = first.get();
    owner moved(std::move(first));
    check(!first && moved.get() == original);
    auto& self = moved;
    moved = std::move(self);
    check(moved.get() == original && a.destroyed == 0);
    second = std::move(moved);
    check(!moved && second.get() == original && b.destroyed == 1);
    first.swap(second);
    check(first.get() == original && !second);
    swap(first, second);
    second.swap(second);
    check(!first && second.get() == original);

    jt::base::vector<owner> owners;
    owners.push_back(std::move(second));
    owners.push_back(
        jt::base::make_dynamic_unique<owner_base, owner_object>(c, 3));
    check(!second && owners[0].get() == original && owners[1]->value() == 3);
    owners[0].reset();
    owners[0].reset();
    check(a.destroyed == 1);
    owners[1] = nullptr;
    check(c.destroyed == 1);

    auto virt =
        jt::base::make_dynamic_unique<owner_base, virtual_owner_object>(v);
    check(virt->value() == 9);
    auto const_owner =
        jt::base::make_dynamic_unique<const owner_base, const owner_object>(
            constant, 4);
    check(const_owner->value() == 4);

    auto root = jt::base::make_dynamic_unique<owner_base, owner_parent>(
        parent,
        jt::base::make_dynamic_unique<owner_base, owner_object>(child, 5));
    auto& member = static_cast<owner_parent*>(root.get())->child;
    root = std::move(member);
    check(parent.destroyed == 1 && child.destroyed == 0 && root->value() == 5);

    const auto prior = jt::base::allocated_memory();
    bool caught = false;
    try {
      auto value = jt::base::make_dynamic_unique<owner_base, owner_object>(
          failed, 6, true);
    } catch (int error) {
      caught = error == 42;
    }
    check(caught && failed.destroyed == 0 &&
          jt::base::allocated_memory() == prior);
  }
  check(a.destroyed == 1 && b.destroyed == 1 && c.destroyed == 1 &&
        v.destroyed == 1 && constant.destroyed == 1 && parent.destroyed == 1 &&
        child.destroyed == 1 && jt::base::allocated_memory() == before);
}

int main() {
  check_ownership();
  const auto before_const = jt::base::allocated_memory();
  const auto destroyed_const = aligned_object<256>::destroyed;
  {
    auto number = jt::base::make_unique<const int>(42);
    auto object = jt::base::make_unique<const aligned_object<256>>();
    check(*number == 42 && object->value == 17);
  }
  check(jt::base::allocated_memory() == before_const &&
        aligned_object<256>::destroyed == destroyed_const + 1);
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
