# 从 main 迁移至当前分支

以下以 `main` 的 `1f83e6d` 为基线，覆盖模块重组、日志 API 和构建配置变化。本次为源码和模块名称的不兼容调整，不提供旧入口、转发模块或类型别名兼容层。共享库符号的模块归属也已变化，下游需要重新编译和链接，不可复用旧 BMI。

## 模块和基础类型

基线以 `jt` 的分区组织模块；当前改为独立命名模块，消费端可继续 `import jt;`，也可按需导入 `jt.base`、`jt.log` 或公开命名模块。

| 原模块分区 | 当前公开模块 |
|---|---|
| `jt:detail.memory` | `jt.base.memory` |
| `jt:detail.buffer` | `jt.base.buffer` |
| `jt:detail.vector/string/deque/unordered_map` | `jt.base.containers` |
| `jt:types.writable_buffer` | `jt.base.concepts` |
| `jt:log.logger`、`jt:log.service` | `jt.log.core` |
| `jt:log.formatter/sink/level/functions` | 对应的 `jt.log.formatter/sink/level/functions` |
| `jt:log.sink.console/file` | `jt.log.sink.console/file` |

表中斜线表示多个模块。若从分支中途的命名模块版本迁移，`jt.detail.memory/buffer` 同样改为 `jt.base.memory/buffer`，容器模块合并至 `jt.base.containers`（含 wstring、unordered_multimap），`jt.types.writable_buffer` 改为 `jt.base.concepts`。中途的 `jt.detail` 聚合入口已移除。

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

`dynamic_deleter<Base>` 和 `make_dynamic_unique<Base, Derived>` 现在要求 Base 有虚析构函数；自定义多态基类需补齐该约束。

## 日志 API

`jt::log` 命名空间和文件 sink 构造方式保留，但调用方需要更新以下接口：

| 基线用法 | 当前用法 |
|---|---|
| `service.start()` | 删除调用；构造 service 即启动工作线程 |
| `service.stop()` | `service.request_stop()` 请求关闭异步提交；service 析构时等待排空、最终 sink 刷新和线程退出 |
| 直接构造 logger | 使用 `service.create_logger(...)`，返回 `std::shared_ptr<logger>` |
| `info(log, fmt, args...)` 等接受 shared_ptr 的辅助函数 | `info(*log, fmt, args...)`；所有级别及 `vinfo` 等运行时格式辅助函数接收 `logger&` |
| `create_logger(name, async, sinks)` 的 vector 重载 | 传入 `std::move(sinks)`；该重载改为按值接收，范围重载仍移动范围元素 |
| `service.post_lz4(...)`、`clear_lz4(...)` | `service.make_lz4_client()` 返回弱句柄，再调用 `post(...)`、`clear(...)` |
| `sink::log(const message&)` | `sink::consume(const log_record_view&)` |
| `formatter::format(const message&, ...)` | `formatter::format(const log_record_view&, ...)`，输出参数改为 `jt::base::buffer_1k&` |

原通用 `jt::log::log`/`vlog` 和带 `sid` 的辅助重载不再是公开 API。固定级别使用 `trace/debug/info/warn/error/critical` 及其 `v` 版本；动态级别或自定义服务 ID 使用 `logger::log(sid, level, buffer, source)`，调用方负责级别筛选、格式化和所需的异常处理。

`message` 及队列字段已成为私有实现。自定义 formatter 将 `sid/tid/point/buf` 分别改为记录视图的 `service_id/thread_id/timestamp/payload`；`lv`、`source` 保留。`payload` 是借用的 `std::string_view`，不能跨调用保留其内容引用。自定义 sink 的 `write` 参数改用 `jt::base::buffer_1k`；`write` 和 `flush_unlock` 是 protected 扩展点，外部使用 `consume` 和 `flush`。

日志时间改为本地时间并输出 UTC 偏移，解析日志的程序需适配。异步 logger 和归档 client 使用弱句柄，保留它们不会延长 service 生命周期；先停止日志生产者，再销毁 service。关闭时会刷新仍存活的异步 logger 已消费但未刷新的 sink，即使 logger 已从注册表移除；无需依赖 logger 析构。`request_stop()` 不等待完成，也不禁用同步 logger；没有公开的 `wait_stop()`。

## 构建

CMake 消费目标推荐链接 `jt::jt`，原目标 `libjt` 保留。删除旧构建缓存/BMI 或使用新的构建目录，然后完整重建。示例源文件从 src 移至 examples，main 目标仍存在，输出位于 `<build>/examples/main`。

`debug`、`release`、`release-with-debug` 预设默认使用 `VCPKG_ROOT` 下的 vcpkg toolchain、仓库 `jt-gcc16` 作为 target 与 host triplet，以及 manifest 依赖。Windows 上 host 工具不再走默认的 `x64-windows`（MSVC）。旧的仅设置 `CXX` 或 `CMAKE_PREFIX_PATH` 的预设用法不再足够；环境设置及自选工具链构建方式见 [README](../README.md#构建)。

本轮未实现 async、net、actor；未来模块约定见 [architecture.md](architecture.md)。当前移除 Asio 强制依赖，mimalloc/LZ4/RapidJSON 仍需安装。

## 独立修复


内存分配新增 `allocate(size, alignment)` 重载，单参数入口保留。分配器和智能指针工厂自动传递实际对象类型的对齐要求（多态工厂使用 `alignof(Derived)`）；调用方式不变，需重建库、BMI 和消费者。

`base_memory_buffer` 自追加在堆扩容时不再读取已释放的来源存储；完整可读视图及已写入区域的子区间均支持。调用方式不变，扩容后仍需重新获取外部持有的指针或视图。模板修复需要重新生成 BMI 并重建消费者。

GCC 16.2 / MinGW 的 `import std` 重复定义通过集中格式化实现及移除不必要的 `condition_variable_any` 修复，不再使用 `--allow-multiple-definition`。新增 `jt.log.format` 公开模块，由 `jt.log` 和 `jt` 再导出；需要自行格式化到 `buffer_1k` 时，可将 `std::format_to(std::back_inserter(buffer), fmt, args...)` 改为 `jt::log::format_to(buffer, fmt, args...)`。运行时格式串使用 `jt::log::vformat_to(buffer, fmt, std::make_format_args(args...))`，参数存储必须在整个调用期间有效。此处同步消费借用参数，异步日志仍提交格式化后的缓冲区。更新后需重新生成 BMI 并重建消费者。

迁移前的文件回归测试暴露了原归档发布错误：`replace_extension` 原地修改临时路径，最终重命名失败仍可能删除源日志。现在保持 `.log.lz4.tmp` 与 `.log.lz4` 路径独立，排他创建临时文件，验证并关闭输出后通过硬链接原子发布，只在成功发布后删除源日志。已有最终归档或临时文件不会被覆盖；发布失败时保留源日志并清理本次创建的临时文件。归档目录所在文件系统需支持硬链接，否则报告失败并保留源日志。此修复不修改轮转和保留策略。

过期归档清理由名称前缀筛选收紧为完整命名格式匹配：`{name}_{YYYYMMDD}[_{seq}].log.lz4`，其中日期为八位数字，序号至少四位数字。`app` 不再清理 `app_worker` 的归档，空名称仅匹配自身日期/序号结构；不符合格式的历史文件需自行管理。公开接口、生成文件名和基于修改时间的保留期限不变。

不同固定容量的 `base_memory_buffer` 之间复制构造和赋值已修复 protected 成员访问错误；调用方式不变，保留读写位置及已写入内容。需重新生成 BMI 并重建消费者。

## 范围与边界修复

`create_logger` 保留 `input_range` 接口并支持独立 sentinel，范围元素仍按顺序移动。`make_sure_writable` 对不可表示的容量抛出 `std::length_error`；分配失败仍为 `std::bad_alloc`，失败后内容、容量及读写位置不变。`read_buffer` 的超长跳过饱和到末尾。

每日轮转将恰好午夜的记录写入新日期文件。归档循环逐条取出请求，不再依赖临时队列分配完成排空或停止。以上无需修改调用方式，但模板变化要求重建库、BMI 及消费者。

## 审核修复

`sink_file_config::keep_days` 新增 1095 天上限（三年按 365 天/年计算），超限构造抛出 `std::invalid_argument`；0 仍禁用清理。直接使用存活的 `lz4_client::clear` 也需遵守上限。

直接调用文件 sink 的 `consume` / `flush` 现在会收到文件打开、写入、刷新和轮转关闭失败的 `std::ios_base::failure`。通过 logger 调用仍隔离异常。后续记录会重新打开出错文件、恢复实际大小并继续轮转；失败记录不自动重放，可能部分写入。

`make_unique<const T>` 可以正常构造、析构及释放。默认格式化器修复首次 epoch、负时间戳的毫秒部分及跨 epoch 缓存；内部缓存布局变化需要重建库和 BMI。


## 清单提交与归档异常恢复

文件轮转先将候选日期和序号写入排他创建的 `manifest_{name}.json.tmp`，校验写入、刷新和关闭后，通过同目录重命名替换正式清单，再更新内存状态并提交旧日志归档。保存失败向直接 sink 调用方抛出异常，保留旧清单和源日志，后续记录可以重试；已有临时文件不会被覆盖。此原子替换不提供断电持久化（fsync）保证。

归档对本次创建的临时文件使用作用域清理，分配等异常退出也会关闭并尝试移除临时文件，使恢复后的请求能够重试；压缩过程异常还会使压缩上下文失效，后续请求重新创建上下文。文件系统本身拒绝删除时仍可能需要人工清理。
