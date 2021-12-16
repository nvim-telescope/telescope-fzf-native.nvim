cmake_minimum_required(VERSION 3.13)
project(fzf C)

set(CMAKE_INSTALL_PREFIX ".")

if (MSVC)
    set(CMAKE_SHARED_LIBRARY_PREFIX lib)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Werror)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED TRUE)
endif()

# To build the library:
#   $ mkdir build && cd build
#   $ cmake .. -DCMAKE_BUILD_TYPE=Release
#   $ cmake --build . --config Release
add_library(fzf SHARED src/fzf.c src/fzf.h)

install(TARGETS fzf DESTINATION ${CMAKE_BINARY_DIR})
