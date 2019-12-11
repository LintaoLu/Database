cmake_minimum_required (VERSION 2.8)
project (Database)

add_subdirectory(indexManager)
add_subdirectory(recordManager)
add_subdirectory (databaseManager)
add_subdirectory(bufferManager)
add_subdirectory(common)
#add_subdirectory (test)

add_dependencies(databaseManager common)
add_dependencies(bufferManager common databaseManager)
add_dependencies(recordManager common databaseManager bufferManager)
add_dependencies(indexManager common databaseManager bufferManager recordManager)

include_directories(indexManager/include)
include_directories(recordManager/include)
include_directories(databaseManager/include)
include_directories(bufferManager/include)
include_directories(test/include)
include_directories(common/include)

enable_testing ()
add_subdirectory (test)

add_dependencies(storageManagerBaseTest databaseManager)

add_dependencies(bufferManagerBaseTest databaseManager bufferManager)
add_dependencies(bufferManagerSecTest databaseManager bufferManager)

add_dependencies(recordManagerBaseTest databaseManager bufferManager recordManager)
add_dependencies(recordManagerExprTest databaseManager bufferManager recordManager)
add_dependencies(recordManagerMarcinTest databaseManager bufferManager recordManager)

add_dependencies(indexManagerBaseTest indexManager)

