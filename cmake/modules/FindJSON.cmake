#
#  Created by Bradley Austin Davis on 2018/07/22
#  Copyright 2013-2018 High Fidelity, Inc.
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# setup hints for JSON search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("json")

# locate header
find_path(JSON_INCLUDE_DIRS "json/json.hpp" HINTS ${JSON_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JSON DEFAULT_MSG JSON_INCLUDE_DIRS)

mark_as_advanced(JSON_INCLUDE_DIRS JSON_SEARCH_DIRS)