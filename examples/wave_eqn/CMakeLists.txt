
include_directories(".")
include_directories("../../include")
include_directories("../../src")
include_directories("${RAJA_DIR}")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -std=c99   -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fopenmp")

set(ENABLE_TBB Off CACHE STRING "")

set(RAJA_USE_CHRONO On CACHE STRING "")

set(RAJA_RANGE_ALIGN           4   )
set(RAJA_RANGE_MIN_LENGTH      32  )
set(RAJA_DATA_ALIGN            64  )
set(RAJA_COHERENCE_BLOCK_SIZE  64  )

add_executable(wave_eqn wave_eqn.cpp)
set_target_properties(wave_eqn PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(wave_eqn apollo)


