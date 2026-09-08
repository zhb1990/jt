if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if(DEFINED ENV{MSYS2_ROOT})
        file(TO_CMAKE_PATH "$ENV{MSYS2_ROOT}" jt_msys2_root)
    else()
        set(jt_msys2_root "C:/msys64")
    endif()
    set(jt_gcc_root "${jt_msys2_root}/ucrt64/bin")
    set(jt_c_compiler "${jt_gcc_root}/gcc.exe")
    set(jt_cxx_compiler "${jt_gcc_root}/g++.exe")

    # Vcpkg identifies MinGW triplets with VCPKG_CMAKE_SYSTEM_NAME=MinGW,
    # while CMake's actual target system name is Windows. Vcpkg's built-in
    # MinGW toolchain normally performs this translation, but this file is a
    # chain-loaded replacement for that toolchain.
    set(CMAKE_SYSTEM_NAME Windows CACHE STRING "Target system" FORCE)
    set(CMAKE_SYSTEM_PROCESSOR x86_64 CACHE STRING "Target processor" FORCE)

    # GCC invokes tools and runtime DLLs from the UCRT64 bin directory. GUI
    # processes do not necessarily inherit the MSYS2 shell's PATH.
    set(ENV{PATH} "${jt_gcc_root};$ENV{PATH}")
else()
    find_program(jt_c_compiler NAMES gcc-16 REQUIRED)
    find_program(jt_cxx_compiler NAMES g++-16 REQUIRED)
endif()

set(CMAKE_C_COMPILER "${jt_c_compiler}" CACHE FILEPATH "GCC 16 C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${jt_cxx_compiler}" CACHE FILEPATH
    "GCC 16 C++ compiler" FORCE)
