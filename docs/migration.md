# 从 jt.detail 迁移至 jt.base

本次为源码和模块名称的不兼容调整，不提供旧入口、转发模块或类型别名兼容层。共享库符号的模块归属也已变化，下游需要重新编译和链接，不可复用旧 BMI。

| 原模块 | 新模块 |
|---|---|
| `jt.detail` | `jt.base` |
| `jt.detail.memory` | `jt.base.memory` |
| `jt.detail.buffer` | `jt.base.buffer` |
| `jt.detail.vector` | `jt.base.containers` |
| `jt.types.writable_buffer` | `jt.base.concepts` |

原内部 string、deque、unordered_map 模块合并至公开 `jt.base.containers`，同时包含 wstring 与 unordered_multimap。

公开基础符号从 `jt::detail` 改为 `jt::base`；`jt::types::writable_buffer` 改为 `jt::base::writable_buffer`。不要全局替换内部的 `jt::detail`：队列、OS、CPU 和统计实现仍使用该命名空间和 PRIVATE 命名模块。

```cpp
import std;
import jt.base;
import jt.log;

jt::base::buffer_1k buffer;
buffer.append("hello");
auto pointer = jt::base::make_unique<int>(42);
jt::base::vector<int> values{1, 2, 3};
static_assert(jt::base::writable_buffer<decltype(buffer)>);
```

`import jt;` 继续可用，固定导出 base 和 log。所有日志公开模块名、`jt::log` 命名空间、create_logger、日志辅助函数及文件 sink 构造方式保留。自定义 sink/formatter 覆盖函数中的 `jt::detail::buffer_1k` 必须改为 `jt::base::buffer_1k`。

CMake 消费目标推荐链接 `jt::jt`，原目标 `libjt` 保留。删除旧构建缓存/BMI 或使用新的构建目录，然后完整重建。示例源文件从 src 移至 examples，main 目标仍存在，输出位于 `<build>/examples/main`。

本轮未实现 async、net、actor；未来模块约定见 architecture.md。当前移除 Asio 强制依赖，mimalloc/LZ4/RapidJSON 仍需安装。

## 独立修复

GCC 16.2 / MinGW 的 `import std` 重复定义通过集中格式化实现及移除不必要的 `condition_variable_any` 修复，不再使用 `--allow-multiple-definition`。现有日志调用方式不变。新增 `jt.log.format` 公开模块，由 `jt.log` 和 `jt` 再导出；需要自行格式化到 `buffer_1k` 时，可将 `std::format_to(std::back_inserter(buffer), fmt, args...)` 改为 `jt::log::format_to(buffer, fmt, args...)`。运行时格式串使用 `jt::log::vformat_to(buffer, fmt, std::make_format_args(args...))`，参数存储必须在整个调用期间有效。此处同步消费借用参数，异步日志仍提交格式化后的缓冲区。更新后需重新生成 BMI 并重建消费者。

迁移前的文件回归测试暴露了原归档发布错误：`replace_extension` 原地修改临时路径，最终重命名失败仍可能删除源日志。现在保持 `.log.lz4.tmp` 与 `.log.lz4` 路径独立，验证并关闭输出后重命名，只在成功发布后删除源日志。重命名失败时保留源日志与临时归档供排查。此修复不修改轮转和保留策略。
