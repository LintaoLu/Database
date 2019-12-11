project(indexManager)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/common/include)
include_directories(${CMAKE_SOURCE_DIR}/databaseManager/include)
include_directories(${CMAKE_SOURCE_DIR}/bufferManager/include)
include_directories(${CMAKE_SOURCE_DIR}/recordManager/include)

link_directories(${CMAKE_BINARY_DIR}/common)
link_directories(${CMAKE_BINARY_DIR}/databaseManager)
link_directories(${CMAKE_BINARY_DIR}/bufferManager)
link_directories(${CMAKE_BINARY_DIR}/recordManager)

set(INDEXMANAGER_SRC include/btree_mgr.h src/btree_mgr.c include/Implement_btree.h src/Implement_btree.c)

add_library(indexManager SHARED ${INDEXMANAGER_SRC})
target_link_libraries(indexManager recordManager common databaseManager bufferManager)