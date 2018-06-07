#
#  FindEtc2Comp.cmake
#
#  Try to find the Etc2Comp compression library.
#
#  Once done this will define
#
#  ETC2COMP_FOUND - system found Etc2Comp
#  ETC2COMP_INCLUDE_DIRS - the Etc2Comp include directory
#  ETC2COMP_LIBRARIES - link to this to use Etc2Comp
#
#  Created on 5/2/2018 by Sam Gondelman
#  Copyright 2018 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("etc2comp")

find_path(ETC_INCLUDE_DIR NAMES Etc.h HINTS ${ETC2COMP_SEARCH_DIRS})
find_path(ETCCODEC_INCLUDE_DIR NAMES EtcBlock4x4.h HINTS ${ETC2COMP_SEARCH_DIRS})
set(ETC2COMP_INCLUDE_DIRS "${ETC_INCLUDE_DIR}" "${ETCCODEC_INCLUDE_DIR}")

find_library(ETC2COMP_LIBRARY_DEBUG NAMES ETC2COMP ETC2COMP_LIB PATH_SUFFIXES EtcLib/Debug HINTS ${ETC2COMP_SEARCH_DIRS})
find_library(ETC2COMP_LIBRARY_RELEASE NAMES ETC2COMP ETC2COMP_LIB PATH_SUFFIXES EtcLib/Release EtcLib HINTS ${ETC2COMP_SEARCH_DIRS})

include(SelectLibraryConfigurations)
select_library_configurations(ETC2COMP)

set(ETC2COMP_LIBRARIES ${ETC2COMP_LIBRARY})

find_package_handle_standard_args(ETC2COMP "Could NOT find ETC2COMP, try to set the path to ETC2COMP root folder in the system variable ETC2COMP_ROOT_DIR or create a directory etc2comp in HIFI_LIB_DIR and paste the necessary files there"
 ETC2COMP_INCLUDE_DIRS ETC2COMP_LIBRARIES)

mark_as_advanced(ETC2COMP_INCLUDE_DIRS ETC2COMP_LIBRARIES ETC2COMP_SEARCH_DIRS)
