cmake_minimum_required(VERSION 3.0)
project(delaunay CXX)
set(CMAKE_CXX_STANDARD 14)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_executable(
    delaunay
        src/delaunay.cpp
)
target_include_directories(
    delaunay PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/delaunator-cpp/include/
)
target_link_libraries(
    delaunay PRIVATE 
        ${VTK_LIBRARIES}
)