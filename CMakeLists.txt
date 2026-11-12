cmake_minimum_required(VERSION 3.15)
project(NetEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(NetEngine STATIC ${SOURCES})
target_link_libraries(NetEngine PRIVATE ws2_32 crypt32)
