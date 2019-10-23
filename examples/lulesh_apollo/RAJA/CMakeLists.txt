###############################################################################
#
# Copyright (c) 2017, Lawrence Livermore National Security, LLC.
#
# Produced at the Lawrence Livermore National Laboratory
#
# All rights reserved.
#
# This file is part of the RAJA Proxy App Suite
#
###############################################################################

set (lulesh_sources
  lulesh.cpp
  lulesh-comm.cpp
  lulesh-init.cpp
  lulesh-util.cpp
  lulesh-viz.cpp)

set (lulesh_depends
  apollo)

# If using MPI, add the include files
if(MPI_FOUND)
	include_directories(${MPI_INCLUDE_PATH})
    link_libraries(${MPI_CXX_LIBRARIES})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MPI_C_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
    # We need C++ because of sosd_spawn.cpp
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_CXX_LINK_FLAGS}")
endif(MPI_FOUND)



set (RAJA_ENABLE_APOLLO On)
include_directories("${RAJA_DIR}")
include_directories("../../../include")

if (ENABLE_OPENMP)
  set (lulesh_depends
    ${lulesh_depends}
    openmp)
endif ()

if (ENABLE_CUDA)
  set (lulesh_depends
    ${lulesh_depends}
    cuda)
endif ()

add_executable(
  lulesh_apollo
  ${lulesh_sources})
add_dependencies(
  lulesh_apollo
  ${lulesh_depends}
  apollo)

target_compile_options(
  lulesh_apollo
  PRIVATE
  -Wno-unknown-pragmas)

target_compile_definitions(
  lulesh_apollo
  PRIVATE
  -DUSE_MPI=1)

target_compile_definitions(
  lulesh_apollo
  PRIVATE
  -DUSE_CASE=128)

target_compile_definitions(
  lulesh_apollo
  PUBLIC
  -DLULESH_DEVICE=
)

if (ENABLE_CUDA)
  configure_file(
    lulesh.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-cuda.cpp COPYONLY)

  configure_file(
    lulesh-comm.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-comm-cuda.cpp COPYONLY)
  configure_file(
    lulesh-init.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-init-cuda.cpp COPYONLY)
  configure_file(
    lulesh-util.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-util-cuda.cpp COPYONLY)
  configure_file(
    lulesh-viz.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-viz-cuda.cpp COPYONLY)

  add_executable(
    lulesh_apollo_cuda
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-cuda.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-comm-cuda.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-init-cuda.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-util-cuda.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lulesh-viz-cuda.cpp)
  add_dependencies(
    lulesh_apollo_cuda
    ${lulesh_depends})

  target_compile_definitions(
    lulesh_apollo_cuda
    PRIVATE
    -DUSE_CASE=9)

  target_compile_definitions(
    lulesh_apollo_cuda
    PRIVATE
    -DUSE_MPI=1)

  target_compile_definitions(
    lulesh_apollo_cuda
    PUBLIC
    -DLULESH_DEVICE=__device__
  )

  target_include_directories(
    lulesh_apollo_cuda
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
