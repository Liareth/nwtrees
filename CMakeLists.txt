cmake_minimum_required(VERSION 3.20)
project(nwtrees VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

function(target_set_options target)
    target_compile_options(${target} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic>)
endfunction()

file(GLOB_RECURSE NWTREES_CORE_SRC src/nwtrees/*.cpp src/nwtrees/*.hpp)
add_library(nwtrees_core ${NWTREES_CORE_SRC})
target_set_options(nwtrees_core)

file(GLOB_RECURSE NWTREES_COMPILER_SRC src/compiler/*.cpp src/compiler/*.hpp)
add_executable(nwtrees ${NWTREES_COMPILER_SRC})
target_link_libraries(nwtrees nwtrees_core)
target_set_options(nwtrees)

file(GLOB_RECURSE NWTREES_TESTS_SRC tests/*.cpp tests/*.hpp)
add_executable(nwtrees_tests ${NWTREES_TESTS_SRC})
target_link_libraries(nwtrees_tests nwtrees_core)
target_set_options(nwtrees_tests)
