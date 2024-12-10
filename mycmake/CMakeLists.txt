cmake_minimum_required(VERSION 3.16)

project(test CXX)

add_executable(${PROJECT_NAME} ../test2.cpp)
my_set_common_properties(${PROJECT_NAME})
target_precompile_headers(${PROJECT_NAME} REUSE_FROM mydhtrecord)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out
    OUTPUT_NAME "${PROJECT_NAME}"
)



