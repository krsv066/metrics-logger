cmake_minimum_required(VERSION 3.27)
project(metrics_logger)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

add_library(metrics_logger INTERFACE)
target_include_directories(metrics_logger INTERFACE include)

add_executable(examples_and_tests examples_and_tests/main.cpp)
target_link_libraries(examples_and_tests metrics_logger)
target_compile_options(examples_and_tests PRIVATE)
target_link_options(examples_and_tests PRIVATE)

set_target_properties(examples_and_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
