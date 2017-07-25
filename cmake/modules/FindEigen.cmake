#
#  FindEigen.cmake
#
#  Try to find Eigen include path.
#  Once done this will define
#
#  EIGEN_INCLUDE_DIRS
#
#  Created on 7/14/2017 by Anthony Thibault
#  Copyright 2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

# setup hints for Eigen search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("eigen")

# locate dir
string(CONCAT EIGEN_INCLUDE_DIRS ${EIGEN_INCLUDE_DIRS} "/src/eigen")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EIGEN DEFAULT_MSG EIGEN_INCLUDE_DIRS)

mark_as_advanced(EIGEN_INCLUDE_DIRS EIGEN_SEARCH_DIRS)