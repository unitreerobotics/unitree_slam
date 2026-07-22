project(slam_example)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/usr/local/include include)

find_package(unitree_sdk2 REQUIRED)

add_executable(keyDemo src/keyDemo.cpp)
target_link_libraries(keyDemo unitree_sdk2)
