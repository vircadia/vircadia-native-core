# 
#  FindFaceshift.cmake
# 
#  Try to find the Faceshift networking library
#
#  You must provide a FACESHIFT_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  FACESHIFT_FOUND - system found Faceshift
#  FACESHIFT_INCLUDE_DIRS - the Faceshift include directory
#  FACESHIFT_LIBRARIES - Link this to use Faceshift
#
#  Created on 8/30/2013 by Andrzej Kapolka
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include(SelectLibraryConfigurations)
select_library_configurations(FACESHIFT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Faceshift DEFAULT_MSG FACESHIFT_INCLUDE_DIRS FACESHIFT_LIBRARIES)
mark_as_advanced(FACESHIFT_INCLUDE_DIRS FACESHIFT_LIBRARIES FACESHIFT_SEARCH_DIRS)