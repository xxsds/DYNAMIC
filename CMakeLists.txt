cmake_minimum_required(VERSION 2.6)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

#set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.. )

project (dynamic)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/internal)
include_directories(${PROJECT_SOURCE_DIR}/include/algorithms)
include_directories(${PROJECT_SOURCE_DIR}/include/algorithms/cw-bwt)

message("Building in ${CMAKE_BUILD_TYPE} mode")

set(CMAKE_CXX_FLAGS "--std=c++11")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -g -p")
set(CMAKE_CXX_FLAGS_RELEASE "-ggdb -Ofast -fstrict-aliasing -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -ggdb -Ofast -fstrict-aliasing -march=native")

add_executable(debug debug.cpp)
add_executable(debug-alan debug-alan.cpp)
add_executable(debug-alan-vec debug-alan-vec.cpp)
add_executable(rle_lz77_v1 rle_lz77_v1.cpp)
add_executable(rle_lz77_v2 rle_lz77_v2.cpp)
add_executable(h0_lz77 h0_lz77.cpp)
add_executable(rle_bwt rle_bwt.cpp)
add_executable(cw-bwt cw-bwt.cpp)
add_executable(benchmark benchmark.cpp)