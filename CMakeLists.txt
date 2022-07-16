cmake_minimum_required(VERSION 3.2)
project(fzf LANGUAGES C)

add_library(${PROJECT_NAME} SHARED "src/fzf.c")

set(is-windows $<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
target_compile_options(${PROJECT_NAME}
  PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall>)
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    $<${is-windows}:_CRT_NONSTDC_NO_DEPRECATE>
    $<${is-windows}:_CRT_SECURE_NO_DEPRECATE>
    $<${is-windows}:_CRT_SECURE_NO_WARNINGS>)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    C_STANDARD 99
    PREFIX lib
    SUFFIX .so)

# This cannot be a generator expression in this version of CMake
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set_property(TARGET ${PROJECT_NAME} PROPERTY SUFFIX .so)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR})
