cmake_minimum_required(VERSION 3.8.1)
project(sqlite_store)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/config)
include_directories("${PROJECT_SOURCE_DIR}/include")
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(sqlite_store_VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_path(SQLITE_MODERN_CPP_INCLUDE_DIRS "sqlite_modern_cpp.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_library(sqlite_store "src/object_collection.cxx"
                         "src/object_store.cxx")
target_link_libraries(sqlite_store PUBLIC  nlohmann_json
                                           nlohmann_json::nlohmann_json
                                   PRIVATE unofficial::sqlite3::sqlite3
                                           fmt::fmt
                                           pthread)
target_include_directories(sqlite_store PRIVATE ${SQLITE_MODERN_CPP_INCLUDE_DIRS})

configure_package_config_file(cmake/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sqlite-store-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/sqlite-store
    NO_SET_AND_CHECK_MACRO)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/sqlite-store-config-version.cmake
    VERSION ${sqlite_store_VERSION}
    COMPATIBILITY SameMajorVersion)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/sqlite-store-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/sqlite-store-config-version.cmake
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/sqlite-store)

install(TARGETS sqlite_store EXPORT sqlite-store-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sqlite_store)
install(EXPORT sqlite-store-targets
    NAMESPACE sqlite-store::
    DESTINATION ${CMAKE_INSTALL_DATADIR}/sqlite-store)
