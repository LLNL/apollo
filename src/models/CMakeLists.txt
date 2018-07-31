
include_directories("..")

#
# NOTE: See if these are necessary, if they've been previously established.
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -std=c99   -ldl")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -ldl")
#

set (BUILTIN_MODELS
    random
    sequential)

foreach(_model ${BUILTIN_MODELS})
    add_library("${_model}Model" "${_model}.cpp")
    set_target_properties("${_model}Model" PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries("${_model}Model" apollo dl)
    #
    install(TARGETS "${_model}Model"
        EXPORT "${_model}Model"
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
    #
endforeach()


