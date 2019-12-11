project(databaseManager)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/common/include)
include_directories(${CMAKE_SOURCE_DIR}/databaseManager/include)
include_directories(${CMAKE_SOURCE_DIR}/bufferManager/include)

link_directories(${CMAKE_BINARY_DIR}/common)
link_directories(${CMAKE_BINARY_DIR}/databaseManager)
link_directories(${CMAKE_BINARY_DIR}/bufferManager)

set(RECORDMANAGER_SRC include/expr.h include/record_mgr.h include/tables.h src/expr.c src/rm_serializer.c src/record_mgr.c include/custom_structs.h)

add_library(recordManager SHARED ${RECORDMANAGER_SRC})
target_link_libraries(recordManager common databaseManager bufferManager)