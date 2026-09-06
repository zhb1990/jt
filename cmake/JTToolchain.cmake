if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "JT requires a 64-bit toolchain")
endif()
if(NOT CMAKE_GENERATOR MATCHES "^Ninja")
    message(FATAL_ERROR "JT uses import std; configure with Ninja or Ninja Multi-Config")
endif()
if(NOT "23" IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR
        "The selected compiler/standard library does not support C++23 import std. "
        "Use a supported GCC 15+, LLVM Clang 18.1.2+, or MSVC 14.36+ combination.")
endif()
