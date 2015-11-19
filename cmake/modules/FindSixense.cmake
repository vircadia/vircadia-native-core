# 
#  FindSixense.cmake 
# 
#  Try to find the Sixense controller library
#
#  You must provide a SIXENSE_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  SIXENSE_FOUND - system found Sixense
#  SIXENSE_INCLUDE_DIRS - the Sixense include directory
#  SIXENSE_LIBRARIES - Link this to use Sixense
#
#  Created on 11/15/2013 by Andrzej Kapolka
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include(SelectLibraryConfigurations)
select_library_configurations(SIXENSE)

set(SIXENSE_REQUIREMENTS SIXENSE_INCLUDE_DIRS SIXENSE_LIBRARIES)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sixense DEFAULT_MSG SIXENSE_INCLUDE_DIRS SIXENSE_LIBRARIES)
mark_as_advanced(SIXENSE_LIBRARIES SIXENSE_INCLUDE_DIRS SIXENSE_SEARCH_DIRS)
