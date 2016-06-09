#
#  FindSteamworks.cmake
#
#  Try to find the Steamworks controller library
#
#  This module defines the following variables
#
#  STEAMWORKS_FOUND - Was Steamworks found
#  STEAMWORKS_INCLUDE_DIRS - the Steamworks include directory
#  STEAMWORKS_LIBRARIES - Link this to use Steamworks
#
#  This module accepts the following variables
#
#  STEAMWORKS_ROOT - Can be set to steamworks install path or Windows build path
#
#  Created on 6/8/2016 by Clement Brisset
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include(SelectLibraryConfigurations)
select_library_configurations(STEAMWORKS)

set(STEAMWORKS_REQUIREMENTS STEAMWORKS_INCLUDE_DIRS STEAMWORKS_LIBRARIES)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Steamworks DEFAULT_MSG STEAMWORKS_INCLUDE_DIRS STEAMWORKS_LIBRARIES)
mark_as_advanced(STEAMWORKS_LIBRARIES STEAMWORKS_INCLUDE_DIRS STEAMWORKS_SEARCH_DIRS)
