# 重组验证记录

验证日期：2026-09-06 至 2026-09-07。新增的 Windows 验证单列如下；后续重组及性能对比记录来自原 macOS arm64 环境。

## mimalloc 3.3.2 的 GCC/macOS 模块兼容修复（2026-09-08）

- macOS arm64、GCC 16.2.0 / libstdc++、macOS 26 SDK、CMake 4.4.3、Ninja 1.13.2。
- 从官方 `v3.3.2` 标签构建 mimalloc 静态库，使用 GCC 16 C 编译器、Release、`MI_OVERRIDE=OFF`；依赖及日志保存在忽略的 `build/deps-mi3`，未替换现有 vcpkg 安装。
- 修复前，在独立 `build/mi3-debug` 目录复现 `memory.cpp` 编译失败：`isascii`、`__istype`、`toupper` 报 `conflicting language linkage for imported declaration`。mimalloc 3.3.2 的 `<wchar.h>` 传递包含 macOS ctype 声明，随后加载 `std` 模块产生冲突。
- 修复将 mimalloc 头文件及三项调用移至普通翻译单元 `memory_backend.cpp`；模块通过无系统头文件的私有声明调用，公开 API、异常及内存统计逻辑不变。
- 修复后 `build/mi3-debug`、新建的 `build/mi3-release` 完整构建成功，分别通过 23/23 CTest。原 `build/debug` 使用 mimalloc 2.2.6 增量重建并通过 23/23 CTest。
- Release 示例在临时目录运行成功，退出时 JT 内存统计为 0；Release 基准同步、异步各处理 20,000 条，`retained_bytes` 均为 0。基准仅作功能验证，未测量桥接调用的性能影响。
- 新增桥接函数未出现在 Release dylib 的外部已定义符号列表中。相关 C++ 文件通过 clang-format 检查，`git diff --check` 通过。本次未重新验证 Windows/Linux，未覆盖 mimalloc 3.3.2 动态链接或其他 3.x 版本。

复现及修复验证使用以下配置；Release 将构建目录换为 `build/mi3-release`、类型换为 `Release`，并追加 `-DJT_BUILD_BENCHMARKS=ON`：

```sh
cmake -S . -B build/mi3-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-16 \
  -DCMAKE_PREFIX_PATH="$HOME/dev/vcpkg/installed/jt-gcc16" \
  -Dmimalloc_DIR="$PWD/build/deps-mi3/install/lib/cmake/mimalloc-3.3"
cmake --build build/mi3-debug --parallel 4
ctest --test-dir build/mi3-debug --output-on-failure
```

上述依赖路径仅描述本机验证环境；构建及测试日志为 `build/deps-mi3/before.log`、`after-{debug,release,v2}.log`、`test-{debug,release,v2}.log`。

## Windows MinGW 重复定义修复（2026-09-07）

- Windows x64、MSYS2 UCRT64 GCC 16.2.0 Rev3 / libstdc++、GNU ld 2.47.20260726、CMake 4.4.3、Ninja。
- 使用本机已有 vcpkg `x64-windows` 包：mimalloc 3.3.2、LZ4 1.10.0 和 RapidJSON。仅记录本机实测组合，不据此保证任意 MSVC/MinGW C++ 二进制混用。
- 修复前复现：`__replace_rep` 同时由 archive、file、default_formatter 产生；`__tag` 同时由 service 和 service_impl 产生。后者来自归档条件变量构造时隐式 `make_shared<mutex>`。
- `build/debug` 与 `build/release` 均从新目录配置并完整构建，分别通过 23/23 CTest；库与消费者均未启用 `--allow-multiple-definition`。原有 `build` Debug 目录也通过全部测试。
- 新增双翻译单元消费测试，覆盖整数/浮点/字符串 debug 格式、自定义 `std::formatter`、运行时格式串、异常捕获、4096 字节消息、调用点位置、本地时间/时区偏移和默认 formatter；公开模块消费测试增加 `jt.log.format`。
- Debug 目标文件检查确认库内 `__replace_rep` 仅在 `format.cpp.obj` 定义，`__tag` 仅在 `service.cpp.obj` 定义。
- 示例首次在隔离目录运行暴露了原有 Windows DLL 复制遗漏：LZ4 未随 libjt 复制到消费目录。补齐 libjt 私有运行库复制后，Release 示例在独立目录运行成功，最终 JT 内存统计为 0。
- Release 基准运行成功，同步、异步各处理 20,000 条，结束时 JT 存量增量均为 0。单次运行仅作功能检查，不与原 macOS 数据比较性能。
- 新增及拆出的 C++ 文件通过 clang-format 检查，`git diff --check` 通过。本次未重新验证 Linux/macOS，未执行 TSAN/ASAN。

首次配置使用 `--preset debug` / `--preset release`，并指定 `CMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe`、本机 Ninja 路径、`CMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake` 和 `VCPKG_TARGET_TRIPLET=x64-windows`。构建进程 PATH 包含 UCRT64 bin；之后运行 `cmake --build --preset <配置> --parallel 4` 和 `ctest --preset <配置>`。这些绝对路径仅描述本机环境。

本地诊断输出保存在忽略的 `build/fix-*.txt` 与 `build/link-diagnosis.txt` 中。

后续 DLL 复制修复：用户报告向 `build/tests` 复制 mimalloc 失败，检查时源文件和目录均存在。生成规则中多个测试目标会并发复制同一 DLL，且合并列表重复列出 mimalloc。现已去重、使用 `copy_if_different`，并串行化 Windows 链接及其部署步骤（编译仍并行），覆盖 vcpkg applocal 与 JT 复制之间的竞争。原 `build` 目录以 `cmake --build build --parallel 24` 构建成功，23/23 CTest 通过；生成规则检查确认部署目标使用深度为 1 的链接任务池。

## 原 macOS 重组验证：已完成

- GCC 16.2.0 / libstdc++、CMake 4.4.3、Ninja：Debug 干净构建通过，21/21 CTest 通过。
- 同一 GCC 工具链的 Release 构建通过，21/21 CTest 通过；测试使用显式检查，未依赖被 NDEBUG 关闭的 assert。
- Debug 与 Release 重复增量构建均返回 `ninja: no work to do.`。
- Release 示例在隔离临时目录运行成功，退出时 JT 跟踪的内存为 0。
- 检查了 30 个模块单元：命名唯一，项目内导入均可解析；没有 async/net/actor 占位目录。
- 新后端、容器聚合、测试和基准源文件通过 clang-format 检查；git diff --check 通过。

测试组成：2 个行为/文件测试、15 个独立公开命名模块消费测试、1 个基础 API 实例化测试、3 个拒绝导入测试。

行为测试覆盖：分配释放与构造异常清理、缓冲区复制移动及内联/堆切换、消费者自定义 sink/formatter 的 RTTI 与虚析构、注册表与默认 logger、同步级别过滤、运行时格式串、调用点信息、4 个生产者的消息完整性与单生产者 FIFO、flush、停止竞态和服务销毁后的异步 no-op。

文件测试覆盖：大小/日期轮转、manifest 重启续写、LZ4 解压回读、归档排空、保留期限与文件名前缀筛选、过期弱句柄、发布失败时保留源日志。文件均放在临时目录。

## 迁移前发现的归档错误

迁移前行为测试通过；文件测试因遗留 `.tmp` 失败。原代码将临时路径就地改为最终路径，再尝试重命名，并忽略错误删除源日志。重组后独立修复此错误；成功归档和失败保留源文件两条路径均有回归测试。

## 工具链与平台限制

- 本机 Homebrew Clang 23.1.0 首次配置无法自动找到 libc++ 模块元数据。显式提供 `CMAKE_CXX_STDLIB_MODULES_JSON` 和匹配的 libc++ 头文件/链接目录后可配置，但完整构建仍失败：该 libc++ 缺少 `std::atomic<std::shared_ptr<T>>` 特化和 `std::chrono::zoned_time`。现有代码在重组前已使用这些能力，没有通过改变原子操作或时间语义绕过限制。
- 原 vcpkg mimalloc 2.2.6 由 Apple Clang 编译。GCC Release 链接它时出现 `std::bad_alloc::bad_alloc()` 未定义；重构前源码同样失败。用 GCC 重建同版本 mimalloc 后，两者均能链接。依赖构建产物全部位于忽略的 build 目录，未修改 vcpkg 安装。
- GCC Debug 使用原 vcpkg 依赖；Release 对比双方均使用 GCC 重建的 mimalloc。因此不跨 Debug/Release 比较性能。
- 原 macOS 重组验证未覆盖 Windows、Linux、x64 和 MinGW；Windows MinGW 的后续结果见本文开头，Linux 仍未验证。
- 线程部分启动失败的回收路径进行了代码检查，但未用故障注入强制触发系统线程创建失败。未执行 TSAN 或 ASAN。

本机 Release 依赖覆盖方式：

```sh
cmake --preset release -Dmimalloc_DIR="$PWD/build/deps-gcc/lib/cmake/mimalloc"
cmake --build --preset release
ctest --preset release
```

`deps-gcc` 是本次本地验证构建的依赖包，不随仓库提交；新环境应安装工具链匹配的依赖。不要将此路径作为通用构建前提。

## 性能对比

相同 macOS arm64、GCC 16.2.0、Release `-O3 -DNDEBUG`、GCC 构建的 mimalloc 2.2.6。迁移前源码从基线提交 `f7f3533` 提取到隔离构建目录，加入同一份基准程序，仅调整基础命名空间。

每轮分别运行重构前后版本，轮次之间交换顺序，共 5 轮；每个版本分别提交 20,000 条同步与异步日志。sink 计数，保留默认格式化工作。吞吐统计包含服务创建、写入和停止排空；逐次计时本身也有开销。这是短程序回归基线，不是生产负载容量评估。

以下为 5 轮中位数：

| 指标 | 重构前同步 | 重构后同步 | 重构前异步 | 重构后异步 |
|---|---:|---:|---:|---:|
| 吞吐，条/秒 | 1,149,359 | 1,216,693 | 1,157,006 | 1,231,148 |
| 提交 p50，ns | 1,000 | 1,000 | 0* | 0* |
| 提交 p99，ns | 1,000 | 1,000 | 2,000 | 2,000 |
| 提交结束时 JT 存量字节 | 298,608 | 298,608 | 12,842,608 | 11,035,248 |
| 服务与 logger 释放后 JT 存量增量 | 0 | 0 | 0 | 0 |

每轮处理数量均为 20,000，无丢失。此组样本未观察到吞吐下降，但不据此声称重组带来确定的性能提升。

`*` 本机该时钟测量呈微秒级量化，0 表示低于测量粒度，不表示调用没有耗时。异步存量字节受消费者调度和积压影响，既不是峰值，也不是分配次数。现有公开内存统计没有累计分配次数接口；累计次数由下面的隔离探针单独测量，不把存量字节误报为分配次数。

原始数据见 [performance-comparison.json](performance-comparison.json)。`benchmarks/log.cpp` 可复现当前版本的基准；完整原始基线与构建产物只保留在忽略的 build 目录。

## 累计分配次数

为避免向正式 API 或热路径增加诊断开销，另外在两个隔离源码副本中给 JT `allocate` 的成功分配路径增加 relaxed 原子计数，并通过仅存在于诊断构建的 C 函数读取。重构前计数入口为 `jt::detail::allocate`，重构后为 `jt::base::allocate`。使用与上表相同的 GCC、Release 和 mimalloc 包。

在每个模式创建 service 之前读计数，在 service 与 logger 释放后再次读取；包括服务初始化及该模式的 20,000 条日志。每个版本运行 3 轮，结果完全一致：

| 模式 | 重构前 JT 成功分配次数 | 重构后 JT 成功分配次数 |
|---|---:|---:|
| 同步 | 15 | 15 |
| 异步 | 20,015 | 20,015 |

计数范围只包含通过 JT 分配入口的调用，不包含标准库、LZ4 或其他直接分配。本基准中的同步日志使用内联缓冲；异步模式每条消息增加一次分配，与保留的实现方式一致。

探针源码和二进制全部位于忽略的 build 目录，正式库、基准程序及公共模块均未添加探针或诊断符号。带探针的时间结果不用于上面的吞吐比较。原始次数记录见 [allocation-comparison.json](allocation-comparison.json)。
