cmake_minimum_required(VERSION 3.13)

include(FetchContent)

FetchContent_Declare(
	novelrt
	
	GIT_REPOSITORY https://github.com/novelrt/NovelRT.git)

FetchContent_GetProperties(novelrt)
if(NOT novelrt_POPULATED)
	FetchContent_Populate(novelrt)
	add_subdirectory(${novelrt_SOURCE_DIR} ${novelrt_BINARY_DIR})
	add_definitions(-DNOVELRT_SKIP_SAMPLES -DNOVELRT_SKIP_TESTS -DNOVELRT_SKIP_DOCUMENTATION -DCMAKE_BUILD_TYPE=Debug)
endif()