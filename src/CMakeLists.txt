include_directories(${NovelChip8_SOURCE_DIR}/deps/novelrt/include ${NovelChip8_SOURCE_DIR}/include)
set(SOURCES CPU.cpp main.cpp ${CMAKE_SOURCE_DIR}/include/CPU.h)

add_executable(Chip8 ${SOURCES})
target_link_libraries(Chip8 NovelRT)