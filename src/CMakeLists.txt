
#  Copyright (c) 2019, Lawrence Livermore National Security, LLC.
#  Produced at the Lawrence Livermore National Laboratory
#
#  This file is part of Apollo.
#  OCEC-17-092
#  All rights reserved.
#
#  Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
#  of many collaborators.
#
#  Apollo was originally created by David Beckingsale, david@llnl.gov
#
#  For details, see https://github.com/LLNL/apollo.
#
#  Permission is hereby granted, free of charge, to any person obtaining
#  a copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.

include_directories(${PROJECT_BINARY_DIR})
include_directories(${CALIPER_INCLUDE_DIR})
include_directories(
    ../include/external/cxxopts/
    )

set(APOLLO_HEADERS
    ../include/apollo/Apollo.h
    ../include/apollo/Config.h
    ../include/apollo/Logging.h
    ../include/apollo/Region.h
    ../include/apollo/PolicyModel.h
    ../include/apollo/TimingModel.h
    ../include/apollo/ModelFactory.h
    )

set(APOLLO_SOURCES
    Apollo.cpp
    Region.cpp
    ModelFactory.cpp
    Config.cpp
    models/Random.cpp
    models/Sequential.cpp
    models/Static.cpp
    models/RoundRobin.cpp
    models/DecisionTree.cpp
    models/RegressionTree.cpp
    models/Optimal.cpp
    )

add_library(apollo SHARED ${APOLLO_SOURCES})

if (ENABLE_MPI)
    target_link_libraries(apollo PUBLIC MPI::MPI_CXX)
endif()

target_link_libraries(apollo PRIVATE dl ${OpenCV_LIBS})

foreach(_extlib ${APOLLO_EXTERNAL_LIBS})
    target_link_libraries(apollo PRIVATE ${_extlib})
endforeach()

install(FILES ${APOLLO_HEADERS} DESTINATION include/apollo)

install(TARGETS apollo
    EXPORT apollo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
