# 
#  TBBLibCopy.cmake
#  cmake/externals/tbb
# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 18, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

# first find the so files in the source dir
file(GLOB_RECURSE _TBB_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/build/*.${TBB_LIBS_SUFFIX}")

# raise an error if we found none
if (NOT _TBB_LIBRARIES)
  message(FATAL_ERROR "Did not find any compiled TBB libraries")
endif ()

# make the libs directory and copy the resulting files there
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
message(STATUS "Copying TBB Android libs to ${CMAKE_CURRENT_SOURCE_DIR}/lib")
file(COPY ${_TBB_LIBRARIES} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/lib")