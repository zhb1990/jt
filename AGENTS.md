# JT Framework

## Build and verify

- Toolchain/dependencies: [README.md](README.md). Tested configurations: [docs/validation.md](docs/validation.md).
- Run `cmake --preset debug`, `cmake --build --preset debug`, and `ctest --preset debug`.
- Run examples in a temporary working directory; they write logs.

## Development rules

- Follow `.clang-format` and `snake_case`; use the existing `jt::base` containers and smart pointers.
- Keep public module interfaces and template dependencies consumer-accessible; do not import PRIVATE modules from public interfaces.
- Register modules with `jt_modules`; configure executables with `jt_configure_executable`.
- Preserve exception safety and `noexcept` destruction. Do not change queue algorithms, memory ordering, or buffer semantics as incidental cleanup.
- Consult [docs/architecture.md](docs/architecture.md) when changing module boundaries, platform support, or architecture; do not add speculative modules/options.
- Update relevant documentation with every change, keeping instructions and descriptions consistent with the code: [README.md](README.md), [docs/architecture.md](docs/architecture.md), [docs/migration.md](docs/migration.md).
