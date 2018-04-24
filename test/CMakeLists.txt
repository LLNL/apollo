include_directories ("../src")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

add_executable(apollo-test apollo-test.cpp)

set_target_properties(apollo-test PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(apollo-test apollo)
