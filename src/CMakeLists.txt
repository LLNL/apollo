# Copyright (c) 2021, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory
# See the top-level LICENSE file for details
# SPDX-License-Identifier: MIT

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
    helpers/OutputFormatter.cpp
    models/Random.cpp
    models/Static.cpp
    models/RoundRobin.cpp
    models/DecisionTree.cpp
    models/RandomForest.cpp
    models/PolicyNet.cpp
    models/impl/DecisionTreeImpl.cpp
    models/impl/RandomForestImpl.cpp
    models/preprocessing/Preprocessing.cpp
    models/Optimal.cpp
    connectors/kokkos/kokkos-connector.cpp
    )

if (ENABLE_OPENCV)
    list(APPEND APOLLO_SOURCES
        models/RegressionTree.cpp
        )
endif()

add_library(apollo SHARED ${APOLLO_SOURCES})

if (ENABLE_MPI)
    target_link_libraries(apollo PUBLIC MPI::MPI_CXX)
endif()

if (ENABLE_OPENCV)
    target_link_libraries(apollo PRIVATE dl ${OpenCV_LIBS})
endif()

if (ENABLE_JIT_DTREE)
    target_link_libraries(apollo PRIVATE ${CMAKE_DL_LIBS})
endif()

foreach(_extlib ${APOLLO_EXTERNAL_LIBS})
    target_link_libraries(apollo PRIVATE ${_extlib})
endforeach()

install(FILES ${APOLLO_HEADERS} DESTINATION include/apollo)

install(TARGETS apollo
    EXPORT apollo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
