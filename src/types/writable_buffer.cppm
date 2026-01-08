export module jt:types.writable_buffer;

import std;

export namespace jt::types {

template <typename Buffer>
concept writable_buffer = requires(Buffer& b, const void* buf,
                                   std::size_t len) { b.append(buf, len); };

}  // namespace jt::types