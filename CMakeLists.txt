cmake_minimum_required(VERSION 3.21.0)
project(kokkos_stream_mdrange VERSION 1.0.0 LANGUAGES CXX)

# Honor already set CMAKE_CXX_STANDARD.
# Otherwise default to C++17
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

#
# Kokkos
#
# General Kokkos settings
set(Kokkos_ENABLE_COMPILER_WARNINGS "ON" CACHE BOOL "CMakeLists: Kokkos_ENABLE_COMPILER_WARNINGS" FORCE)
set(Kokkos_ENABLE_AGGRESSIVE_VECTORIZATION "ON" CACHE BOOL "CMakeLists: Kokkos_ENABLE_AGGRESSIVE_VECTORIZATION" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/kokkos/kokkos ${CMAKE_CURRENT_BINARY_DIR}/kokkos)

add_executable(stream-kokkos-range stream-kokkos-range.cpp)
target_link_libraries(stream-kokkos-range Kokkos::kokkos)

add_executable(stream-kokkos-2d-mdrange stream-kokkos-2d-mdrange.cpp)
target_link_libraries(stream-kokkos-2d-mdrange Kokkos::kokkos)

add_executable(stream-kokkos-2d-mdrange-tiling-scan stream-kokkos-2d-mdrange-tiling-scan.cpp)
target_link_libraries(stream-kokkos-2d-mdrange-tiling-scan Kokkos::kokkos)

add_executable(stream-kokkos-3d-mdrange stream-kokkos-3d-mdrange.cpp)
target_link_libraries(stream-kokkos-3d-mdrange Kokkos::kokkos)

add_executable(stream-kokkos-4d-mdrange stream-kokkos-4d-mdrange.cpp)
target_link_libraries(stream-kokkos-4d-mdrange Kokkos::kokkos)

add_executable(stream-kokkos-4d-mdrange-tiling stream-kokkos-4d-mdrange-tiling.cpp)
target_link_libraries(stream-kokkos-4d-mdrange-tiling Kokkos::kokkos)

add_executable(stream-kokkos-4d-mdrange-rec-tiling stream-kokkos-4d-mdrange-rec-tiling.cpp)
target_link_libraries(stream-kokkos-4d-mdrange-rec-tiling Kokkos::kokkos)

add_executable(stream-kokkos-4d-openmp stream-kokkos-4d-openmp.cpp)
target_link_libraries(stream-kokkos-4d-openmp Kokkos::kokkos)

add_executable(stream-kokkos-4d-openmp-simd stream-kokkos-4d-openmp-simd.cpp)
target_link_libraries(stream-kokkos-4d-openmp-simd Kokkos::kokkos)

add_executable(stream-kokkos-5d-mdrange stream-kokkos-5d-mdrange.cpp)
target_link_libraries(stream-kokkos-5d-mdrange Kokkos::kokkos)
