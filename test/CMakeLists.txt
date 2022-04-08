# Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
# Apollo project developers. Produced at the Lawrence Livermore National
# Laboratory. See the top-level LICENSE file for details.
# SPDX-License-Identifier: MIT

set(CMAKE_CXX_STANDARD 14)

add_executable(apollo-test apollo-test.cpp)
add_executable(apollo-overhead apollo-overhead.cpp)

target_link_libraries(apollo-test apollo)
target_link_libraries(apollo-overhead apollo)
