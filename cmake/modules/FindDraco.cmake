#
#  FindDraco.cmake
#
#  Try to find Draco libraries and include path.
#  Once done this will define
#
#  DRACO_FOUND
#  DRACO_INCLUDE_DIRS
#  DRACO_LIBRARY
#  DRACO_ENCODER_LIBRARY
#  DRACO_DECODER_LIBRARY
#
#  Created on 8/8/2017 by Stephen Birarda
#  Copyright 2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("draco")

find_path(DRACO_INCLUDE_DIRS draco/core/draco_types.h PATH_SUFFIXES include/draco/src include HINTS ${DRACO_SEARCH_DIRS})

find_library(DRACO_LIBRARY draco PATH_SUFFIXES "lib" HINTS ${DRACO_SEARCH_DIRS})
find_library(DRACO_ENCODER_LIBRARY draco PATH_SUFFIXES "lib" HINTS ${DRACO_SEARCH_DIRS})
find_library(DRACO_DECODER_LIBRARY draco PATH_SUFFIXES "lib" HINTS ${DRACO_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DRACO DEFAULT_MSG DRACO_INCLUDE_DIRS DRACO_LIBRARY DRACO_ENCODER_LIBRARY DRACO_DECODER_LIBRARY)
