cmake_minimum_required(VERSION 3.10)
project(fzf C)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Werror)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED TRUE)
endif()

# To build the library:
#   $ mkdir build && cd build
#   $ cmake .. -DCMAKE_BUILD_TYPE=Release
#   $ cmake --build .
add_library(fzf SHARED src/fzf.c src/fzf.h)
