cmake_minimum_required(VERSION 3.10)
project(memorylib C)

set(CMAKE_C_STANDARD 11)
option(BUILD_TESTS "Build test programs" OFF)

include_directories(.)

add_library(memorylib STATIC src/memory.c)

set_target_properties(memorylib PROPERTIES PREFIX "")

if (BUILD_TESTS)
	enable_testing()
	file(GLOB_RECURSE TESTFILES "test/*.c")
	foreach(TESTFILE ${TESTFILES})
		get_filename_component(TESTFILE_STRIPPED ${TESTFILE} NAME_WE)
		add_executable(${TESTFILE_STRIPPED} ${TESTFILE})
		target_link_libraries(${TESTFILE_STRIPPED} memorylib)
		add_test(NAME ${TESTFILE_STRIPPED} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TESTFILE_STRIPPED}) 
	endforeach()
endif()