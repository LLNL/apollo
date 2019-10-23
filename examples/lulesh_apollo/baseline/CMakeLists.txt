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
# If using MPI, add the include files
if(MPI_FOUND)
	include_directories(${MPI_INCLUDE_PATH})
    link_libraries(${MPI_CXX_LIBRARIES})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MPI_C_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
    # We need C++ because of sosd_spawn.cpp
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_CXX_LINK_FLAGS}")
endif(MPI_FOUND)


set (lulesh_depends
  apollo)

# We need to include RAJA with the same features it was built with, even if
# they are not used.
if (ENABLE_OPENMP)
  set (lulesh_depends
    ${lulesh_depends}
    openmp)
endif ()

set (lulesh_sources
  lulesh.cpp
  lulesh-comm.cpp
  lulesh-init.cpp
  lulesh-util.cpp
  lulesh-viz.cpp)

add_executable(
  lulesh_apollo_base
  ${lulesh_sources})
add_dependencies(
  lulesh_apollo_base
  ${lulesh_depends})

target_compile_definitions(
  lulesh_apollo_base
  PRIVATE
  -DUSE_MPI=1)

target_compile_definitions(
  lulesh_apollo_base
  PRIVATE
  -DUSE_OMP=0)

target_compile_options(
  lulesh_apollo_base
  PRIVATE
  -Wno-unknown-pragmas)

target_compile_definitions(
  lulesh_apollo_base
  PRIVATE)

if(ENABLE_OPENMP)
  add_executable(
    lulesh_apollo_base_omp
    ${lulesh_sources})
  add_dependencies(
    lulesh_apollo_base_omp
    openmp)

  target_compile_definitions(
    lulesh_apollo_base_omp
    PRIVATE
    -DUSE_MPI=1)

  target_compile_definitions(
    lulesh_apollo_base_omp
    PRIVATE
    -DUSE_OMP=1)

endif()
