
# Copyright (c) 2019, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory
#
# This file is part of Apollo.
# OCEC-17-092
# All rights reserved.
#
# Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
# of many collaborators.
#
# Apollo was originally created by David Beckingsale, david@llnl.gov
#
# For details, see https://github.com/LLNL/apollo.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

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
