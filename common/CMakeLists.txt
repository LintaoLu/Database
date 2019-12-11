project(common)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)

set(COMMON_SRC include/dt.h include/dberror.h src/dberror.c)

add_library(common SHARED ${COMMON_SRC})