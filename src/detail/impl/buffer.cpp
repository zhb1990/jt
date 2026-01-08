module;

#include "../config.h"

// module jt:detail.buffer;
module jt;

namespace jt::detail {

template class JT_API base_memory_buffer<1024>;
template class JT_API base_memory_buffer<2048>;
template class JT_API base_memory_buffer<4096>;
template class JT_API base_memory_buffer<8192>;

}  // namespace jt::detail