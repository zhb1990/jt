# AGENTS.md — JT Framework

## Build and verification

- CMake >= 4.3, Ninja >= 1.11, 64-bit C++23 toolchain with working `import std`.
- Dependencies: mimalloc, LZ4, RapidJSON CMake packages. Asio is not currently used.
- `cmake --preset debug`, `cmake --build --preset debug`, `ctest --preset debug`.
- Select the compiler and dependency toolchain on first configure; see README.md.
- `release` presets enable benchmarks. Run `build/release/benchmarks/jt_log_benchmark`.
- `main` remains the example target; output is `<build>/examples/main`. Run in a temporary working directory because it writes logs.
- Tests cover behavior, file rotation/archives, module consumers and rejected private imports.
- Current options: BUILD_TESTING, JT_BUILD_EXAMPLES, JT_BUILD_BENCHMARKS.
- Supported platform intent: macOS/Linux x64 and arm64, Windows x64. Consult docs/validation.md for combinations actually tested.

## Source layout

- `src/jt.cppm`: umbrella re-exporting jt.base and jt.log.
- `src/base`: public memory, buffer, containers, concepts; adjacent interface and implementation files.
- `src/detail/platform`, `concurrency`, `metrics`: cross-domain PRIVATE modules.
- `src/log/core`: public logger/service partitions and implementation units.
- `src/log/core/detail`: message, service_impl, writer_backend, archive_worker implementation partitions.
- `src/log/sinks`: independent sink, console and file named modules.
- `src/log/detail`: private default formatter.
- Domain CMakeLists files contribute to the same libjt shared target. Consumer alias: jt::jt.
- `examples`, `tests`, `benchmarks`, `docs`, `cmake` have separate responsibilities.

## C++23 modules

- Public entry points: `import jt;`, `import jt.base;`, `import jt.log;`, or the documented public named modules.
- Public foundation modules: jt.base.memory, jt.base.buffer, jt.base.containers, jt.base.concepts.
- No public jt.detail umbrella or compatibility aliases remain.
- Register modules with `jt_modules(PUBLIC ...)` or `jt_modules(PRIVATE ...)`; ordinary implementation sources use target_sources PRIVATE.
- Public interfaces must not import PRIVATE modules. Public templates must have consumer-accessible dependencies.
- Umbrella modules only export-import independent named modules. jt.log.core exports its own :fwd/:logger/:service interface partitions.
- Keep logger/service in jt.log.core; shared public forward declarations live in :fwd. Keep the import graph acyclic.
- formatter, sink and derived sinks stay independent named modules to preserve the GCC Darwin typeinfo workaround.
- Internal cross-module named interfaces use `export module jt.detail.*;` in PRIVATE CXX_MODULES file sets.
- Core internal partitions use `module jt.log.core:message;`, `:service_impl`, `:writer`, `:archive` in PRIVATE file sets.
- Definitions belong to the declaring module: `module jt.log.core;`, `module jt.log.sink;`, `module jt.base.buffer;`, etc. Do not use implementation partition declarations for ordinary implementation units.
- Import std before other imports. Imports precede exported declarations.
- JT_API interfaces start with a global module fragment including the platform/config.h file, then export module.
- Third-party and OS headers belong in global module fragments or internal implementation units.

## Shared library / platforms

- Target libjt retains PREFIX ""; jt::jt is an alias.
- Windows/Cygwin: JT_DLL_EXPORT is library-only; BMIs bake it in. Consumers must not redefine import/export macros.
- POSIX: JT_LIB_VISIBILITY, hidden default symbols and inlines. PUBLIC module sources get -fvisibility=default so GCC module initializers (_ZGIW*) are exported.
- MinGW GCC: preserve stdc++exp and runtime DLL copying.
- Use jt_configure_executable for examples/tests/benchmarks so Windows runtime DLLs are colocated.

## Style and ownership

- Follow .clang-format (Google style, 2 spaces); snake_case for classes, methods, variables and aliases.
- Public foundation namespace: jt::base. Logging: jt::log. Foundation internals: jt::detail.
- Use std explicit-width integer types and std::string_view for borrowed text.
- Use jt::base::allocator-backed containers and custom unique_ptr/dynamic_unique_ptr. dynamic_deleter requires a virtual base destructor.
- Preserve exception containment in logging, noexcept destruction and low-level semantics.
- Keep template definitions in reachable module interfaces; place non-template implementation next to its interface.
- Do not rewrite queue algorithms, memory ordering or buffer behavior as incidental cleanup.

## Logging lifecycle

- Construct jt::log::service to start workers; request_stop closes async submissions. Destruction also waits for workers.
- Only service::create_logger constructs logger; it takes a movable sink range and returns std::shared_ptr<logger>.
- Helpers take logger&, e.g. jt::log::info(*log, "value {}", value); runtime format uses vinfo/vwarn/etc.
- Levels: trace, debug, info, warn, error, critical. Preserve source_location call-site capture.
- service_impl owns registry and coordinates lifetime. writer_backend owns queue/submission/dispatch; archive_worker owns compression/retention.
- Shutdown: close submissions -> drain writer -> notify archive stop -> drain archive -> join. Preserve startup-failure cleanup.
- writer_backend has private logger backend access through friendship; do not export backend methods.
- logger backend links and service::lz4_client are weak handles. Retaining logger does not retain service.
- File sink owns rotation and manifest state. Only remove source logs after a complete archive has been successfully published.

## Future architecture (not yet implemented)

- jt.async, jt.net and jt.actor all depend on jt.log and jt.base. Logging never depends on these domains.
- net depends on async; actor depends on async but not net; actor.net bridges both.
- Inject std::shared_ptr<jt::log::logger> at component construction. Application owns the log service and destroys it last.
- Actor is optional; generic async/network usage must not require it.
- Add JT_ENABLE_ASYNC/NET/ACTOR only when implementing those domains, default OFF. NET/ACTOR require ASYNC; both NET and ACTOR enable actor.net.
- Do not add placeholder modules or invent scheduler/message wire formats during structural work.
- Update docs/architecture.md, docs/migration.md and README.md with public changes.
