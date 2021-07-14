# Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
# Apollo Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)


include_directories(${PROJECT_BINARY_DIR})
include_directories(${CALIPER_INCLUDE_DIR})
include_directories(
    ../include/external/cxxopts/
    )
include_directories(
  ./connectors/kokkos/include
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
    connectors/kokkos/kokkos-connector.cpp
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
