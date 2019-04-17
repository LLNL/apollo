if (${CMAKE_CXX_COMPILER_ID} MATCHES Intel)
  # CMake does not have proper compiler feature support for Intel compilers :-/
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -ldl -fPIC")
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -std=c99   -ldl -fPIC")
else()
  set(CMAKE_C_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_STANDARD_REQUIRED ON) 

  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_C_STANDARD   99)
endif()

include_directories(${PROJECT_BINARY_DIR})
include_directories(${CALIPER_INCLUDE_DIR})
include_directories(
    ../include/external/nlohmann/
    ../include/external/cxxopts/
    )

set(APOLLO_HEADERS
    ../include/apollo/Apollo.h
    ../include/apollo/ModelWrapper.h
    ../include/apollo/Region.h
    ../include/apollo/Model.h
    ../include/apollo/models/Random.h
    ../include/apollo/models/Sequential.h
    ../include/apollo/models/Static.h
    ../include/apollo/models/DecisionTree.h
    )

set(APOLLO_SOURCES
    Apollo.cpp
    Region.cpp
    ModelWrapper.cpp
    models/Random.cpp
    models/Sequential.cpp
    models/Static.cpp
    models/DecisionTree.cpp
    )

add_library(apollo SHARED ${APOLLO_SOURCES})

target_link_libraries(apollo dl)

foreach(_extlib ${APOLLO_EXTERNAL_LIBS})
    target_link_libraries(apollo ${_extlib})
endforeach()

install(FILES ${APOLLO_HEADERS} DESTINATION include/apollo)

install(TARGETS apollo
    EXPORT apollo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
