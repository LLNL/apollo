include_directories ("../../src")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

find_package(RAJA REQUIRED)

add_executable(dotprod dotprod.cpp)

set_target_properties(dotprod PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(dotprod RAJA apollo)
