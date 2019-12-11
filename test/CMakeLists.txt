cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

include_directories (include)
include_directories (${CMAKE_SOURCE_DIR}/common/include)
include_directories (${CMAKE_SOURCE_DIR}/databaseManager/include)
include_directories(${CMAKE_SOURCE_DIR}/bufferManager/include)
include_directories(${CMAKE_SOURCE_DIR}/recordManager/include)
include_directories(${CMAKE_SOURCE_DIR}/indexManager/include)

link_directories(${CMAKE_BINARY_DIR}/common)
link_directories(${CMAKE_BINARY_DIR}/bufferManager)
link_directories(${CMAKE_BINARY_DIR}/databaseManager)
link_directories(${CMAKE_BINARY_DIR}/recordManager)
link_directories(${CMAKE_BINARY_DIR}/indexManager)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(COMMON_TEST_SRC include/test_helper.h)
set(DATABASEMANAGER_SRC_BASE_TEST src/databaseManager/test_assign1_1.c)
set(BUFFERMANAGER_SRC_BASE_TEST src/bufferManager/test_assign2_1.c)
set(BUFFERMANAGER_SRC_SEC_TEST src/bufferManager/test_assign2_2.c)
set(RECORDMANAGER_SRC_BASE_TEST src/recordManager/test_assign3_1.c)
set(RECORDMANAGER_SRC_EXPR_TEST src/recordManager/test_expr.c)
set(RECORDMANAGER_SRC_MARCIN_TEST src/recordManager/marcin_test.c)
set(INDEXMANAGER_SRC_BASE_TEST src/indexManager/test_assign4_1.c)

add_executable(storageManagerBaseTest ${COMMON_TEST_SRC} ${DATABASEMANAGER_SRC_BASE_TEST})
add_executable(bufferManagerBaseTest ${COMMON_TEST_SRC} ${BUFFERMANAGER_SRC_BASE_TEST})
add_executable(bufferManagerSecTest ${COMMON_TEST_SRC} ${BUFFERMANAGER_SRC_SEC_TEST})
add_executable(recordManagerBaseTest ${COMMON_TEST_SRC} ${RECORDMANAGER_SRC_BASE_TEST})
add_executable(recordManagerExprTest ${COMMON_TEST_SRC} ${RECORDMANAGER_SRC_EXPR_TEST})
add_executable(recordManagerMarcinTest ${COMMON_TEST_SRC} ${RECORDMANAGER_SRC_MARCIN_TEST})
add_executable(indexManagerBaseTest ${COMMON_TEST_SRC} ${INDEXMANAGER_SRC_BASE_TEST})

target_link_libraries(storageManagerBaseTest -lcommon -ldatabaseManager)
target_link_libraries(bufferManagerBaseTest -lcommon -ldatabaseManager -lbufferManager)
target_link_libraries(bufferManagerSecTest -lcommon -ldatabaseManager -lbufferManager)
target_link_libraries(recordManagerBaseTest -lcommon -ldatabaseManager -lbufferManager -lrecordManager)
target_link_libraries(recordManagerExprTest -lcommon -ldatabaseManager -lbufferManager -lrecordManager)
target_link_libraries(recordManagerMarcinTest -lcommon -ldatabaseManager -lbufferManager -lrecordManager)
target_link_libraries(indexManagerBaseTest -lcommon -ldatabaseManager -lbufferManager -lrecordManager -lindexManager)

add_test(NAME storageManagerBaseTest COMMAND storageManagerBaseTest)

add_test(NAME bufferManagerBaseTest COMMAND bufferManagerBaseTest)
add_test(NAME bufferManagerSecTest COMMAND bufferManagerSecTest)

add_test(NAME recordManagerBaseTest COMMAND recordManagerBaseTest)
add_test(NAME recordManagerExprTest COMMAND recordManagerExprTest)
add_test(NAME recordManagerMarcinTest COMMAND recordManagerMarcinTest)

add_test(NAME indexManagerBaseTest COMMAND indexManagerBaseTest)