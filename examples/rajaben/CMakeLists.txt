
include_directories(".")
include_directories("../../include")
include_directories("../../src")
include_directories("${RAJA_DIR}/include")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -std=c99   -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fopenmp")

set(ENABLE_TBB Off CACHE Bool "")

set(RAJA_USE_CHRONO On CACHE Bool "")

set(RAJA_RANGE_ALIGN           4   )
set(RAJA_RANGE_MIN_LENGTH      32  )
set(RAJA_DATA_ALIGN            64  )
set(RAJA_COHERENCE_BLOCK_SIZE  64  )

add_executable(rajaben rajaben.cpp)
set_target_properties(rajaben PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(rajaben apollo)


