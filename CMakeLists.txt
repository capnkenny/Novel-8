cmake_minimum_required(VERSION 3.13)

project(NovelChip8 VERSION 0.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64")

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (MSVC)
  add_compile_options(/W4 /WX /wd4611)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-pedantic -pedantic-errors -Wall -Wextra -Werror -Wno-float-equal -Wno-padded)
else()
  add_compile_options(-pedantic -pedantic-errors -Wall -Wextra -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-float-equal -Wno-padded -Wno-reserved-id-macro)
endif()

add_subdirectory(deps)
add_subdirectory(src)

target_include_directories(Chip8 PRIVATE ${CMAKE_SOURCE_DIR}/include)