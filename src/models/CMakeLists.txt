#
#  # NOTE: This is deprecated, models are not built as API-conformant independent
#  #       shared objects, but as specific classes baked into libapollo.so
#
#  include_directories("..")
#  
#  #
#  # NOTE: See if these are necessary, if they've been previously established.
#  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -std=c99   -ldl -fPIC ")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -ldl -fPIC ")
#  #
#  
#  set (BUILTIN_MODELS
#      Random 
#      Sequential
#      Static
#      DecisionTree
#      Python
#      )
#  
#  foreach(_model ${BUILTIN_MODELS})
#      add_library("${_model}Model" SHARED "${_model}.cpp")
#      set_target_properties("${_model}Model" PROPERTIES
#          LINKER_LANGUAGE CXX
#          PREFIX "")
#      target_link_libraries("${_model}Model" apollo )
#      #
#      install(TARGETS "${_model}Model"
#          EXPORT "${_model}Model"
#          ARCHIVE DESTINATION lib/models
#          LIBRARY DESTINATION lib/models)
#      #
#  endforeach()
#  
#  
