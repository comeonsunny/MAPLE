set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

cmake_minimum_required(VERSION 2.8)
project(MAPLE)
set(CMAKE_CXX_STANDARD 14)
#find_path(NTL_INCLUDE_DIR NAMES ZZ.h PATH_SUFFIXES NTL)
#include_directories(${NTL_INCLUDE_DIR})
find_path(ZMQ_INCLUDE_DIR NAMES zmq.h PATH_SUFFIXES zmq)
include_directories(${ZMQ_INCLUDE_DIR})
include_directories(/usr/local/include)
add_library(pms3_src
    src/MAPLE.cpp
    src/blockMAPLE.cpp
    src/clientMAPLE.cpp
    src/serverMAPLE.cpp
    src/Utils.cpp
    src/Log.cpp
)

add_executable(MAPLE_v2 main.cpp)
target_link_libraries(MAPLE_v2 pms3_src -lntl -lgmp -lzmq -lpthread -lstdc++fs)