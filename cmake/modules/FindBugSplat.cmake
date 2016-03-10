#
#  FindBugSplat.cmake
#  cmake/modules
#
#  Created by Ryan Huffman on 02/09/16.
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if (WIN32)
  include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
  hifi_library_search_hints("BugSplat")

  find_path(BUGSPLAT_INCLUDE_DIRS NAMES BugSplat.h PATH_SUFFIXES inc HINTS ${BUGSPLAT_SEARCH_DIRS})

  find_library(BUGSPLAT_LIBRARY_RELEASE "BugSplat64.lib" PATH_SUFFIXES "lib64" HINTS ${BUGSPLAT_SEARCH_DIRS})
  find_path(BUGSPLAT_DLL_PATH NAMES "BugSplat64.dll" PATH_SUFFIXES "bin64" HINTS ${BUGSPLAT_SEARCH_DIRS})
  find_file(BUGSPLAT_RC_DLL_PATH NAMES "BugSplatRc64.dll" PATH_SUFFIXES "bin64" HINTS ${BUGSPLAT_SEARCH_DIRS})
  find_file(BUGSPLAT_EXE_PATH NAMES "BsSndRpt64.exe" PATH_SUFFIXES "bin64" HINTS ${BUGSPLAT_SEARCH_DIRS})

  include(SelectLibraryConfigurations)
  select_library_configurations(BUGSPLAT)

  set(BUGSPLAT_LIBRARIES ${BUGSPLAT_LIBRARY_RELEASE})
endif ()

set(BUGSPLAT_REQUIREMENTS BUGSPLAT_INCLUDE_DIRS BUGSPLAT_LIBRARIES BUGSPLAT_DLL_PATH BUGSPLAT_RC_DLL_PATH BUGSPLAT_EXE_PATH)
find_package_handle_standard_args(BugSplat DEFAULT_MSG ${BUGSPLAT_REQUIREMENTS})
