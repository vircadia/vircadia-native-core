#
#  FindGLI.cmake
# 
#  Try to find GLI include path.
#  Once done this will define
#
#  GLI_INCLUDE_DIRS
# 
#  Created on 2016/09/03 by Bradley Austin Davis
#  Copyright 2013-2016 High Fidelity, Inc.
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# setup hints for GLI search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("gli")

# locate header
find_path(GLI_INCLUDE_DIRS "gli/gli.hpp" HINTS ${GLI_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLI DEFAULT_MSG GLI_INCLUDE_DIRS)

mark_as_advanced(GLI_INCLUDE_DIRS GLI_SEARCH_DIRS)