
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

add_executable(omp_print omp_print.c)
set_target_properties(omp_print PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(omp_print apollo)

install(TARGETS
    omp_print
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
