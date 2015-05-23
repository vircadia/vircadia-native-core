# 
#  Try to find BOOSTCONFIG include path.
#  Once done this will define
#
#  BOOSTCONFIG_INCLUDE_DIRS
# 
#  Created by Bradley Austin Davis on 2015/05/22
#  Copyright 2015 High Fidelity, Inc.
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# setup hints for BOOSTCONFIG search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("BOOSTCONFIG")

# locate header
find_path(BOOSTCONFIG_INCLUDE_DIRS "boost/config.hpp" HINTS ${BOOSTCONFIG_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BOOSTCONFIG DEFAULT_MSG BOOSTCONFIG_INCLUDE_DIRS)

mark_as_advanced(BOOSTCONFIG_INCLUDE_DIRS BOOSTCONFIG_SEARCH_DIRS)