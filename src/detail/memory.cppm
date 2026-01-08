module;

#include <cassert>

#include "config.h"

export module jt:detail.memory;

import std;

namespace jt::detail {

export JT_API auto allocate(std::size_t size) -> void*;

export JT_API auto allocated_size(void* ptr) -> std::size_t;

export JT_API void deallocate(void* ptr, std::size_t size);

export JT_API auto allocated_memory() -> std::int64_t;

template <typename T>
class allocator {
 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_move_assignment = std::true_type;
  using is_always_equal = std::true_type;

  static_assert(!(std::is_const_v<T> || std::is_void_v<T>),
                "The C++ Standard forbids containers of const elements "
                "because allocator<const T> is ill-formed.");

  allocator() = default;

  allocator(const allocator&) = default;

  template <class Other>
  explicit allocator(const allocator<Other>&) noexcept {}

  ~allocator() = default;

  // ReSharper disable once CppMemberFunctionMayBeStatic
  [[nodiscard]] auto allocate(const std::size_t count) -> T* {
    assert(count <= static_cast<std::size_t>(-1) / sizeof(T));
    void* ptr = detail::allocate(sizeof(T) * count);
    return static_cast<T*>(ptr);
  }

  void deallocate(T* const ptr, const std::size_t) { detail::deallocate(ptr); }

  [[nodiscard]] auto allocate(const std::size_t count, const void*) -> T* {
    return this->allocate(count);
  }

  template <class Obj, class... Types>
  void construct(Obj* const ptr, Types&&... args) {
    ::new (const_cast<void*>(static_cast<const volatile void*>(ptr)))
        Obj(std::forward<Types>(args)...);
  }

  template <class Obj>
  // ReSharper disable once CppMemberFunctionMayBeStatic
  void destroy(Obj* const ptr) {
    ptr->~Obj();
  }

  // ReSharper disable once CppMemberFunctionMayBeStatic
  [[nodiscard]] constexpr std::size_t max_size() const noexcept {
    return (static_cast<std::size_t>(-1) - 2 * sizeof(std::size_t)) / sizeof(T);
  }
};

template <class T, class Other>
[[nodiscard]] bool operator==(const allocator<T>&,
                              const allocator<Other>&) noexcept {
  return true;
}

template <class T, class Other>
[[nodiscard]] bool operator!=(const allocator<T>&,
                              const allocator<Other>&) noexcept {
  return false;
}

template <typename T>
  requires(!std::is_array_v<T>)
struct deleter {
  void operator()(T* ptr) const noexcept {
    static_assert(sizeof(*ptr), "can't delete an incomplete type");
    ptr->~T();
    return deallocate(ptr, sizeof(T));
  }
};

template <typename T>
using unique_ptr = std::unique_ptr<T, deleter<T>>;

template <typename T, typename... Types>
  requires(!std::is_array_v<T>)
auto make_unique(Types&&... args) -> unique_ptr<T> {
  auto* ptr = ::new (allocate(sizeof(T))) T(std::forward<Types>(args)...);
  return unique_ptr<T>(ptr, deleter<T>());
}

}  // namespace jt::detail
