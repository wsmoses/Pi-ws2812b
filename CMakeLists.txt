cmake_minimum_required(VERSION 3.0`)

project("neoclient")
set(NEOCLIENT_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
set(NEOCLIENT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

message(STATUS "source dir ${NEOCLIENT_SOURCE_DIR}")
message(STATUS "binary dir ${NEOCLIENT_BINARY_DIR}")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third-party/pybind11)
pybind11_add_module(remotearray src/remotearray.cc)
target_link_libraries(remotearray
    PRIVATE
)

