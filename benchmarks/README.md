# 日志基准

先按 [README 的构建说明](../README.md#构建) 配置 `VCPKG_ROOT` 和 GCC 16；以下预设使用仓库的 `jt-gcc16` triplet。

```sh
cmake --preset release
cmake --build --preset release --target jt_log_benchmark
./build/release/benchmarks/jt_log_benchmark
```

每种模式写入 20,000 条日志到计数 sink，仍使用默认 formatter。程序验证后台最终处理数，报告包含生命周期的吞吐、单条提交 p50/p99、提交结束时 JT 存量字节和服务释放后的存量增量。

`pending_bytes` 不是峰值或累计分配次数。时钟粒度会限制纳秒单位结果的精度。标准库、LZ4 等自行分配的内存不计入 JT 的 allocated_memory。

比较变更时使用同一机器、优化级别、编译器和依赖包，交替运行两个版本。不要将 Debug 与 Release 混比，也不要将一次短测试的吞吐变化当作稳定优化结论。本次验证记录在 docs/validation.md。

本次累计分配次数使用隔离源码探针单独比较，方法和数据见 docs/validation.md；正式基准程序不包含探针。
