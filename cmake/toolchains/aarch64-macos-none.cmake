set(CMAKE_SYSTEM_NAME "Darwin")
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR "aarch64")

set(CMAKE_C_COMPILER "zig" cc -target aarch64-macos-none)
set(CMAKE_CXX_COMPILER "zig" c++ -target aarch64-macos-none)

set(CMAKE_AR "${CMAKE_CURRENT_LIST_DIR}/zig-ar.sh")
set(CMAKE_RANLIB "${CMAKE_CURRENT_LIST_DIR}/zig-ranlib.sh")
