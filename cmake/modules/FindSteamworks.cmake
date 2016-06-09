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

set(STEAMWORKS_POSSIBLE_PATHS "/Users/clement/Downloads/steamworks_sdk_137/")

find_path(STEAMWORKS_INCLUDE_DIRS
    NAMES steam_api.h
    PATH_SUFFIXES "public/steam"
    PATHS ${STEAMWORKS_POSSIBLE_PATHS}
)

get_filename_component(STEAMWORKS_INCLUDE_DIR_NAME ${STEAMWORKS_INCLUDE_DIRS} NAME)
if (STEAMWORKS_INCLUDE_DIR_NAME STREQUAL "steam")
    get_filename_component(STEAMWORKS_INCLUDE_DIRS ${STEAMWORKS_INCLUDE_DIRS} PATH)
else()
    message(STATUS "Include directory not named steam, this will cause issues with include statements")
endif()

find_library(STEAMWORKS_LIBRARIES
    NAMES libsteam_api.dylib
    PATH_SUFFIXES "redistributable_bin/osx32"
    PATHS ${STEAMWORKS_POSSIBLE_PATHS}
)


find_package_handle_standard_args(Steamworks DEFAULT_MSG STEAMWORKS_LIBRARIES STEAMWORKS_INCLUDE_DIRS)