
include_directories(".")
include_directories("../../include")
include_directories("../../src")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -std=c99   -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fopenmp")

set(ENABLE_TBB Off CACHE Bool "")

add_executable(synben synben.cpp)
set_target_properties(synben PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(synben apollo)


