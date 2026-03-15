export module jt:types.writable_buffer;

import std;

export namespace jt::types {

/**
 * 可写缓冲区概念
 * 定义了一个类型必须满足的要求才能被视为可写缓冲区
 * @tparam Buffer 要检查的缓冲区类型
 * @note 该概念要求类型具有append成员函数，接受const void*和std::size_t参数
 */
template <typename Buffer>
concept writable_buffer = requires(Buffer& b, const void* buf,
                                    std::size_t len) { b.append(buf, len); };

}  // namespace jt::types