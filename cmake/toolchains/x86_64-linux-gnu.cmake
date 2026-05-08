set(CMAKE_SYSTEM_NAME "Linux")
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR "x86_64")

set(CMAKE_C_COMPILER "zig" cc -target x86_64-linux-gnu)
set(CMAKE_CXX_COMPILER "zig" c++ -target x86_64-linux-gnu)

set(CMAKE_AR "${CMAKE_CURRENT_LIST_DIR}/zig-ar.sh")
set(CMAKE_RANLIB "${CMAKE_CURRENT_LIST_DIR}/zig-ranlib.sh")
