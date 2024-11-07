cmake_minimum_required(VERSION 3.14)

project(cactusbot)

# add cmake-specific flag to disable 'C with C++ compiler' deprecation warning
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-deprecated")
endif()

# set language
set(CB_LANGUAGE CXX)

# set preferred C++ standard
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE source src/*.c)

set_source_files_properties(${source} PROPERTIES LANGUAGE ${CB_LANGUAGE})

add_executable(cactusbot ${source})
