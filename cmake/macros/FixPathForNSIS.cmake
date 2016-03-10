#
#  FixPathForNSIS.cmake
#
#  Created by Sam Gateau on 1/14/16.
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(fix_path_for_nsis INITIAL_PATH RESULTING_PATH)
  # replace forward slash with backslash
  string(REPLACE "/" "\\\\" _BACKSLASHED_PATH ${INITIAL_PATH})
  # set the resulting path variable
  set(${RESULTING_PATH} ${_BACKSLASHED_PATH})
endmacro()
