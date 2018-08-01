
include_directories("..")

#
# NOTE: See if these are necessary, if they've been previously established.
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -std=c99   -ldl -fPIC ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -ldl -fPIC ")
#

set (BUILTIN_MODELS
    random
    sequential)

foreach(_model ${BUILTIN_MODELS})
    add_library("${_model}Model" SHARED "${_model}.cpp")
    set_target_properties("${_model}Model" PROPERTIES
        LINKER_LANGUAGE CXX
        PREFIX "")
    target_link_libraries("${_model}Model" apollo )
    #target_link_libraries("${_model}Model" -Wl,--whole-archive apollo -Wl,--no-whole-archive)
    #
    install(TARGETS "${_model}Model"
        EXPORT "${_model}Model"
        ARCHIVE DESTINATION lib/models
        LIBRARY DESTINATION lib/models)
    #
endforeach()


