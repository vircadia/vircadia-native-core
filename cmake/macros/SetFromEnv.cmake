# 
#  Created by Bradley Austin Davis on 2017/11/27
#  Copyright 2013-2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

function(set_from_env _RESULT_NAME _ENV_VAR_NAME _DEFAULT_VALUE)
    if (NOT DEFINED ${_RESULT_NAME}) 
        if ("$ENV{${_ENV_VAR_NAME}}" STREQUAL "")
            set (${_RESULT_NAME} ${_DEFAULT_VALUE} PARENT_SCOPE)
        else()
            set (${_RESULT_NAME} $ENV{${_ENV_VAR_NAME}} PARENT_SCOPE)
        endif()
    endif()
endfunction()
