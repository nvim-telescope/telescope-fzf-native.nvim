cmake_minimum_required(VERSION 3.2)
project(fzf)

add_compile_options(-Wall -Werror)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_library(${PROJECT_NAME} SHARED "src/fzf.c")

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else ()
	target_compile_options(${PROJECT_NAME} PRIVATE -std=gnu99)
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR})
