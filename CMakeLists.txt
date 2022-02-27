cmake_minimum_required(VERSION 3.8.1)
project(sqlite_store)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/config)
include_directories("${PROJECT_SOURCE_DIR}/include")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_path(SQLITE_MODERN_CPP_INCLUDE_DIRS "sqlite_modern_cpp.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_library(sqlite_store SHARED "src/object_collection.cxx"
                                "src/object_store.cxx")
target_link_libraries(sqlite_store PUBLIC  nlohmann_json
                                           nlohmann_json::nlohmann_json
                                   PRIVATE unofficial::sqlite3::sqlite3
                                           fmt::fmt
                                           pthread)
target_include_directories(sqlite_store PRIVATE ${SQLITE_MODERN_CPP_INCLUDE_DIRS})
