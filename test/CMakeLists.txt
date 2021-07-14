

include_directories ("../src")

set(CMAKE_CXX_STANDARD 14)

find_package(MPI REQUIRED)

add_executable(apollo-test apollo-test.cpp)

# TODO Enable the target when this is functional
set_target_properties(apollo-test PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(apollo-test apollo MPI::MPI_CXX)
