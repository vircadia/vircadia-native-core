#
#  FindGifCreator.cmake
# 
#  Try to find GifCreator include path.
#  Once done this will define
#
#  GIFCREATOR_INCLUDE_DIRS
# 
#  Created on 11/15/2016 by Zach Fox
#  Copyright 2016 High Fidelity, Inc.
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# setup hints for GifCreator search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("GIFCREATOR")

# locate header
find_path(GIFCREATOR_INCLUDE_DIRS "GifCreator/GifCreator.h" HINTS ${GIFCREATOR_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GifCreator DEFAULT_MSG GIFCREATOR_INCLUDE_DIRS)

mark_as_advanced(GIFCREATOR_INCLUDE_DIRS GIFCREATOR_SEARCH_DIRS)