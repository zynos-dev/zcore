cmake_minimum_required(VERSION 3.25)
project(zcore VERSION 0.1.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(ZCORE_BUILD_TESTS "Build zcore unit tests" ON)
option(ZCORE_BUILD_DOCS "Enable Doxygen API documentation target" OFF)
option(ZCORE_BUILD_BENCHMARKS "Build zcore microbenchmarks" OFF)
option(ZCORE_USE_DOXYGEN_AWESOME "Use doxygen-awesome-css theme for Doxygen output" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(zcore_hash_impl INTERFACE)
target_include_directories(
  zcore_hash_impl
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/xxhash>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(zcore INTERFACE)
add_library(zcore::zcore ALIAS zcore)
target_compile_features(zcore INTERFACE cxx_std_23)
target_include_directories(
  zcore
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  zcore
  INTERFACE
    zcore_hash_impl
    $<$<AND:$<PLATFORM_ID:Linux>,$<CXX_COMPILER_ID:Clang>>:atomic>
)

set(ZCORE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/zcore")

install(
  TARGETS zcore zcore_hash_impl
  EXPORT zcoreTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/xxhash/xxhash.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  EXPORT zcoreTargets
  FILE zcoreTargets.cmake
  NAMESPACE zcore::
  DESTINATION ${ZCORE_INSTALL_CMAKEDIR}
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/zcoreConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/zcoreConfig.cmake
  INSTALL_DESTINATION ${ZCORE_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/zcoreConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/zcoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/zcoreConfigVersion.cmake
  DESTINATION ${ZCORE_INSTALL_CMAKEDIR}
)

if(ZCORE_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ZCORE_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(ZCORE_BUILD_DOCS)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/zcoreDocs.cmake)
  zcore_configure_docs()
endif()
