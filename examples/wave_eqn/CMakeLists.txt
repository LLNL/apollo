
include_directories(".")
include_directories("../../include")
include_directories("../../src")
include_directories("${RAJA_DIR}")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -std=c99   -fopenmp ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fopenmp ")

# If using MPI, add the include files
if(MPI_FOUND)
	include_directories(${MPI_INCLUDE_PATH})
    link_libraries(${MPI_CXX_LIBRARIES})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MPI_C_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
    # We need C++ because of sosd_spawn.cpp
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_CXX_LINK_FLAGS}")
endif(MPI_FOUND)

set(ENABLE_TBB Off CACHE STRING "")

set(RAJA_USE_CHRONO On CACHE STRING "")

set(RAJA_RANGE_ALIGN           4   )
set(RAJA_RANGE_MIN_LENGTH      32  )
set(RAJA_DATA_ALIGN            64  )
set(RAJA_COHERENCE_BLOCK_SIZE  64  )

add_executable(wave_eqn_apollo wave_eqn.cpp)
set_target_properties(wave_eqn_apollo PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(wave_eqn_apollo PRIVATE " -DWAVE_EQN_ENABLE_APOLLO -DRAJA_ENABLE_APOLLO ")
target_link_libraries(wave_eqn_apollo apollo)

add_executable(wave_eqn_normal wave_eqn.cpp)
set_target_properties(wave_eqn_normal PROPERTIES LINKER_LANGUAGE CXX)
