# 架构与模块边界

## 组织原则

目录按能力领域组织，命名模块按依赖边界划分。接口 `.cppm` 与非模板实现 `.cpp` 相邻；领域内部实现放入本领域的 `detail/`，跨领域共享实现放入 `src/detail/`。

一个 `libjt` 共享库承载所有已启用领域，消费别名为 `jt::jt`。各领域 CMake 文件登记源码，不按每个模块创建库。公共接口和公开模板只依赖 PUBLIC 模块；PRIVATE 模块只供实现单元和其他 PRIVATE 模块导入。

## 当前模块

| 公开入口 | 再导出/职责 |
|---|---|
| `jt` | `jt.base`、`jt.log` |
| `jt.base` | `jt.base.memory/buffer/containers/concepts` |
| `jt.base.memory` | 分配、统计 API、自定义分配器、智能指针 |
| `jt.base.buffer` | read_buffer、channel_buffer、base_memory_buffer 和常用实例 |
| `jt.base.containers` | vector、string、wstring、deque、unordered_map、unordered_multimap |
| `jt.base.concepts` | writable_buffer |
| `jt.log` | core、level、record、format、formatter、sink、sink.console、sink.file、functions |
| `jt.log.format` | 向 buffer_1k 追加格式化文本，以及本地时间缓存格式化 |
| `jt.log.core` | `:fwd`、`:logger`、`:service` 接口分区 |
| `jt.log.record` | `log_record_view`，供 formatter 和 sink 消费的只读记录视图 |

`jt::base` 承载公开基础 API，`jt::log` 承载日志 API，`jt::detail` 只包含内部基础实现。内部 `jt.detail.*` 的点号不表示模块继承；不存在公开 `jt.detail` 聚合入口。

`jt.base.memory` 的公开定义及内存统计仍在 `memory.cpp` 模块实现中。mimalloc 调用位于普通翻译单元 `src/base/memory_backend.cpp`，该文件不导入 `std` 或 JT 模块。私有桥接头 `memory_backend.h` 不包含系统头文件，使用 `decltype(sizeof(0))` 表示大小类型；模块仅在 global module fragment 中包含该桥接头。这样将 mimalloc 3.3.2 的 `<wchar.h>` 等传递包含与 GCC 的 `std` 模块隔离，避免 macOS 系统声明的 language linkage 冲突。桥接符号不属于公开模块或 DLL API。

logger 与 service 的前向声明位于 `jt.log.core:fwd`。实现类型的非导出前向声明与使用它的接口保持同一模块归属。`formatter`、`sink` 和派生 sink 保留独立命名模块，避免回退到项目曾遇到的 GCC Darwin 重复 typeinfo 结构。

公开日志与控制台模板依赖 PUBLIC 模块 `jt.log.format`，只构造 `std::format_args`，通过非模板函数调用 `src/log/format.cpp` 中的运行时引擎。该实现单元同时负责 chrono 格式化，因为 chrono formatter 内部也会调用标准库格式化引擎。`format_local_time(timestamp, date_and_time, zone_offset)` 覆盖 128/32 字节内联缓存，按当前时区输出秒精度日期时间及 UTC 偏移。默认 formatter 的非模板成员定义位于相邻的 `detail/default_formatter.cpp`，保留原有缓存和输出行为。

格式化参数仅在当前调用期间借用，不进入异步队列；队列仍保存格式化后的字节。这个边界集中 JT 的标准库实例化，不能修复用户在多个翻译单元直接调用 `std::format`、chrono formatter 或在自定义 formatter 内再次使用标准库格式化引擎时的工具链问题。

队列消息 `message` 是私有实现。公开扩展接口使用 `formatter::format(const log_record_view&, ...)` 和 `sink::consume(const log_record_view&)`；视图中的 `payload` 借用消息内容，若需在调用结束后使用，应自行复制。

## 日志后端

- `service_impl`：logger 注册表、默认 logger 和两个 worker 的生命周期协调。
- `writer_backend`：消息分配、MPSC 队列、提交计数、写入与刷新；通过友元调用 logger 私有后台接口。
- `archive_worker`：归档队列、LZ4 上下文、压缩与过期文件清理。
- 文件 sink：轮转、manifest、文件写入，通过 `service::lz4_client` 提交归档请求。

内部声明使用 `jt.log.core:message/:service_impl/:writer/:archive` 非导出分区，定义使用 `module jt.log.core;`。LZ4 头文件只存在于归档内部单元；RapidJSON 只用于文件 sink 实现。

启动时先构造 worker 状态和压缩上下文，再启动 writer，最后启动 archive。archive 线程创建失败时停止并回收 writer；worker 析构提供额外清理保证。

`service::request_stop()` 关闭异步日志提交。writer 完成正在提交的消息并排空队列后通知 archive 停止，archive 排空已接收请求后退出。service 析构时通过内部 `service_impl::wait_stop()` 按 writer、archive 顺序回收线程；`wait_stop()` 不是公开 API。成员声明保证 archive 比 writer 活得更久，注册表在 worker 回收后才释放。同步日志继续保留现有直接调用 sink 的语义。

logger 消息目标与归档 client 均使用弱引用。只有调用期间临时持有后端；service 销毁后异步提交和归档请求自行失效。请求停止不等于同步 logger 被禁用，也不等于持有 logger 就持有 service。

## 后续领域

以下是设计边界，不是已实现功能。当前不创建目录占位、空模块或无效构建选项。

箭头表示依赖：

```mermaid
flowchart TD
  ActorNet["jt.actor.net"] --> Actor["jt.actor"]
  ActorNet --> Net["jt.net"]
  Actor --> Async["jt.async"]
  Net --> Async
  Actor --> Log["jt.log"]
  Net --> Log
  Async --> Log
  Log --> Base["jt.base"]
  Async --> Base
  Net --> Base
  Actor --> Base
```

- `jt.async`：协程任务、执行器、定时器、取消。不认识 Actor 服务和消息。
- `jt.net`：连接、监听、网络 I/O；使用 async，不负责业务消息分派。
- `jt.actor`：服务标识、生命周期、邮箱、消息、请求响应和服务调度；底层执行资源来自 async，不强制依赖 net。
- `jt.actor.net`：组合 Actor 与网络，将网络事件接入服务。

三个领域都依赖 log。异步执行环境、网络上下文、Actor 运行时在组件实例层面接收并持有 `std::shared_ptr<jt::log::logger>`，内部对象沿用该 logger，不引入日志全局单例。应用先启动日志，后启动其他组件；退出时停止 Actor、网络和异步环境，最后销毁日志 service，保证退出过程可记录日志。

日志继续使用独立后台线程，不反向依赖 async、net 或 actor。

实现顺序为 async → net/actor → actor.net。届时添加默认 OFF 的 `JT_ENABLE_ASYNC/NET/ACTOR`；NET、ACTOR 各自要求 ASYNC，缺失时报配置错误，ACTOR 不要求 NET。NET 与 ACTOR 同时开启时构建 actor.net。关闭 Actor 不影响通用协程和网络。Asio 在异步后端实际落地时按需引入，不直接出现在 JT 公共类型中。

## 构建边界

保留 Windows `JT_DLL_EXPORT`（BMI 固化宏）、POSIX 默认隐藏符号与公共模块初始化符号导出、MinGW stdc++exp 规则；不放宽重复定义检查。归档 worker 使用 `std::condition_variable` 配合 `std::unique_lock<std::mutex>`，避免 `condition_variable_any` 构造时隐式 `make_shared<mutex>` 产生另一份 GCC/MinGW `__tag` 强定义。消费者不得通过重新定义导入宏修改已生成 BMI。各可执行目标复制必要 Windows runtime DLL。

PUBLIC/PRIVATE 是 CMake 文件集可见性，不能仅凭 `export module` 判断是否为用户 API。模板所需依赖必须可被消费者取得；内部平台实现通过实现单元使用，不得从公共接口导入。

Windows 的库及消费目标使用容量为 1 的 Ninja 链接任务池，使链接附带的 vcpkg applocal 和 JT DLL 复制步骤不会在共享输出目录中并发写同一个文件；源码编译仍可并行。JT 合并并去重消费者与 libjt 的运行库列表，使用 `copy_if_different` 复制，避免重复写入 mimalloc 等共同依赖。
