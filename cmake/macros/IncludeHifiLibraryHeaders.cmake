# 
#  IncludeHifiLibraryHeaders.cmake
#  cmake/macros
# 
#  Copyright 2014 High Fidelity, Inc.
#  Created by Stephen Birarda on August 8, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(include_hifi_library_headers LIBRARY ROOT_DIR)
  
  include_directories("${ROOT_DIR}/libraries/${LIBRARY}/src")
  
endmacro(include_hifi_library_headers _library _root_dir)