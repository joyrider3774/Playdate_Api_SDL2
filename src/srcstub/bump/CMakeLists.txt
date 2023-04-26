cmake_minimum_required(VERSION 3.20)

project(bump)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#glob recurse is not ideal, but I am keeping it here because I am too lazy!!!
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "*.h" "*.hpp" "*.cpp")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*tests/.*")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*build/.*")

add_library(bump ${SOURCE_FILES})
target_include_directories(bump PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")




add_subdirectory(tests)
