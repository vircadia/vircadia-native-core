#
#  FindOGLPLUS.cmake
# 
#  Try to find OGLPLUS include path.
#  Once done this will define
#
#  OGLPLUS_INCLUDE_DIRS
# 
#  Created on 1/30/2015 by Brad Davis
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# setup hints for OGLPLUS search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("oglplus")

# locate header
find_path(OGLPLUS_INCLUDE_DIR "oglplus/all.hpp" PATH_SUFFIXES include HINTS ${OGLPLUS_SEARCH_DIRS})

set(OGLPLUS_INCLUDE_DIRS "${OGLPLUS_INCLUDE_DIR}")
list(APPEND OGLPLUS_INCLUDE_DIRS "${OGLPLUS_INCLUDE_DIR}/../implement")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OGLPLUS DEFAULT_MSG OGLPLUS_INCLUDE_DIRS)
mark_as_advanced(OGLPLUS_INCLUDE_DIRS OGLPLUS_SEARCH_DIRS)