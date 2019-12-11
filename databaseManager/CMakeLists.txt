project(databaseManager)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/common/include)

link_directories(${CMAKE_BINARY_DIR}/common)

set(DATABASEMANAGER_SRC include/storage_mgr.h src/storage_mgr.c)

add_library(databaseManager SHARED ${DATABASEMANAGER_SRC})
target_link_libraries (databaseManager common)