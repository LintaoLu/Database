project(databaseManager)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/common/include)
include_directories(${CMAKE_SOURCE_DIR}/databaseManager/include)

link_directories(${CMAKE_BINARY_DIR}/common)
link_directories(${CMAKE_BINARY_DIR}/databaseManager)

set(STATEGY_SRC include/FIFO.h include/LRU.h include/strategy.h source/FIFO.c source/LRU.c source/strategy.c include/Page.h)
set(BUFFERMANAGER_SRC include/buffer_mgr.h include/buffer_mgr_stat.h source/buffer_mgr.c source/buffer_mgr_stat.c)

add_library(bufferManager SHARED ${BUFFERMANAGER_SRC} ${STATEGY_SRC})
#add_library(bufferManager SHARED ${BUFFERMANAGER_SRC})
target_link_libraries(bufferManager common databaseManager)