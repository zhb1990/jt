module;
 
#include <cassert>
 
#include "config.h"
 
export module jt:detail.memory;
 
import std;
 
export namespace jt::detail {
 
/**
 * 原始内存分配函数
 * @param size 要分配的字节数
 * @return 分配的内存指针
 */
JT_API auto allocate(std::size_t size) -> void*;
 
/**
 * 获取已分配内存的实际大小
 * @param ptr 内存指针
 * @return 实际分配的字节数
 */
JT_API auto allocated_size(const void* ptr) -> std::size_t;
 
/**
 * 释放内存
 * @param ptr 要释放的内存指针
 */
JT_API void deallocate(void* ptr);
 
/**
 * 获取当前已分配的总内存量
 * @return 已分配的内存字节数（带符号的64位整数）
 */
JT_API auto allocated_memory() -> std::int64_t;
 
/**
 * 自定义分配器模板
 * 为STL容器提供自定义内存分配策略
 * @tparam T 要分配的对象类型
 */
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
 
   /**
    * 分配指定数量的对象内存
    * @param count 要分配的对象数量
    * @return 分配的内存指针
    */
   // ReSharper disable once CppMemberFunctionMayBeStatic
   [[nodiscard]] auto allocate(const std::size_t count) -> T* {
     assert(count <= static_cast<std::size_t>(-1) / sizeof(T));
     void* ptr = detail::allocate(sizeof(T) * count);
     return static_cast<T*>(ptr);
   }
 
   /**
    * 释放内存
    * @param ptr 要释放的内存指针
    * @param count 未使用的参数（保持与标准分配器接口一致）
    */
   // ReSharper disable once CppMemberFunctionMayBeStatic
   void deallocate(T* const ptr, const std::size_t) { detail::deallocate(ptr); }
 
   /**
    * 分配内存（带提示位置的版本）
    * @param count 要分配的对象数量
    * @param hint 未使用的提示参数
    * @return 分配的内存指针
    */
   [[nodiscard]] auto allocate(const std::size_t count, const void*) -> T* {
     return this->allocate(count);
   }
 
   /**
    * 在已分配的内存中构造对象
    * @param ptr 指向已分配内存的指针
    * @param args 构造函数参数
    */
   template <class Obj, class... Types>
   void construct(Obj* const ptr, Types&&... args) {
     ::new (const_cast<void*>(static_cast<const volatile void*>(ptr)))
         Obj(std::forward<Types>(args)...);
   }
 
   /**
    * 析构对象
    * @param ptr 指向要析构对象的指针
    */
   // ReSharper disable once CppMemberFunctionMayBeStatic
   void destroy(Obj* const ptr) {
     ptr->~Obj();
   }
 
   /**
    * 获取分配器能分配的最大对象数量
    * @return 最大可分配对象数量
    */
   // ReSharper disable once CppMemberFunctionMayBeStatic
   [[nodiscard]] constexpr std::size_t max_size() const noexcept {
     return (static_cast<std::size_t>(-1) - 2 * sizeof(std::size_t)) / sizeof(T);
   }
 };
 
/**
 * 分配器相等比较运算符
 * 由于我们的分配器是无状态的，所以所有分配器实例都相等
 * @tparam T 第一个分配器的对象类型
 * @tparam Other 第二个分配器的对象类型
 * @param lhs 左操作数分配器
 * @param rhs 右操作数分配器
 * @return 始终返回true
 */
template <class T, class Other>
[[nodiscard]] auto operator==(const allocator<T>&,
                              const allocator<Other>&) noexcept -> bool {
   return true;
 }
 
/**
 * 分配器不等比较运算符
 * 由于我们的分配器是无状态的，所以所有分配器实例都相等
 * @tparam T 第一个分配器的对象类型
 * @tparam Other 第二个分配器的对象类型
 * @param lhs 左操作数分配器
 * @param rhs 右操作数分配器
 * @return 始终返回false
 */
template <class T, class Other>
[[nodiscard]] auto operator!=(const allocator<T>&,
                              const allocator<Other>&) noexcept -> bool {
   return false;
 }
 
/**
 * 删除器模板
 * 用于unique_ptr的自定义删除策略
 * @tparam T 要删除的对象类型
 */
template <typename T>
   requires(!std::is_array_v<T>)
struct deleter {
   /**
    * 删除对象
    * @param ptr 要删除的对象指针
    */
   void operator()(T* ptr) const noexcept {
     static_assert(sizeof(*ptr), "can't delete an incomplete type");
     ptr->~T();
     return deallocate(ptr);
   }
 };
 
/**
 * 标准unique_ptr类型别名
 * 使用自定义删除器的unique_ptr
 * @tparam T 要管理的对象类型
 */
template <typename T>
using unique_ptr = std::unique_ptr<T, deleter<T>>;
 
/**
 * make_unique函数模板
 * 使用自定义分配器创建对象并返回unique_ptr
 * @tparam T 要创建的对象类型
 * @tparam Types 构造函数参数类型
 * @param args 构造函数参数
 * @return 管理新创建对象的unique_ptr
 */
template <typename T, typename... Types>
   requires(!std::is_array_v<T>)
auto make_unique(Types&&... args) -> unique_ptr<T> {
   auto* ptr = ::new (allocate(sizeof(T))) T(std::forward<Types>(args)...);
   return unique_ptr<T>(ptr, deleter<T>());
 }
 
/**
 * 动态删除器模板
 * 用于dynamic_unique_ptr的自定义删除策略
 * @tparam Base 基类类型
 */
template <typename Base>
struct dynamic_deleter {
   void* raw_ptr{nullptr};
 
   /**
    * 删除对象
    * @param ptr 要删除的对象指针
    */
   void operator()(Base* ptr) const noexcept {
     ptr->~Base();
     return deallocate(raw_ptr);
   }
 };
 
/**
 * 动态unique_ptr类型别名
 * 用于多态对象的unique_ptr，保存原始指针以便正确释放
 * @tparam Base 基类类型
 */
template <typename Base>
using dynamic_unique_ptr = std::unique_ptr<Base, dynamic_deleter<Base>>;
 
/**
 * make_dynamic_unique函数模板
 * 使用自定义分配器创建派生类对象并返回管理基类的dynamic_unique_ptr
 * @tparam Base 基类类型
 * @tparam Derived 派生类类型（必须是Base的派生类）
 * @tparam Types 构造函数参数类型
 * @param args 构造函数参数
 * @return 管理新创建对象的dynamic_unique_ptr
 */
template <typename Base, typename Derived, typename... Types>
   requires(std::is_base_of_v<Base, Derived>)
auto make_dynamic_unique(Types&&... args) -> dynamic_unique_ptr<Base> {
   auto* ptr =
       ::new (allocate(sizeof(Derived))) Derived(std::forward<Types>(args)...);
   return dynamic_unique_ptr<Base>{dynamic_cast<Base*>(ptr), {ptr}};
 }
 
}  // namespace jt::detail
