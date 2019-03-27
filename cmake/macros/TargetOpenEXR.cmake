#
#  Copyright 2015 High Fidelity, Inc.
#  Created by Olivier Prat on 2019/03/26
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_OPENEXR)
    if (NOT ANDROID)
        set(openexr_config_file "${VCPKG_INSTALL_ROOT}/include/OpenEXR/OpenEXRConfig.h")
        if(EXISTS ${openexr_config_file})
            file(STRINGS
                ${openexr_config_file}
                TMP
                REGEX "#define OPENEXR_VERSION_STRING.*$")
            string(REGEX MATCHALL "[0-9.]+" OPENEXR_VERSION ${TMP})
    
            file(STRINGS
                ${openexr_config_file}
                TMP
                REGEX "#define OPENEXR_VERSION_MAJOR.*$")
            string(REGEX MATCHALL "[0-9]" OPENEXR_MAJOR_VERSION ${TMP})
    
            file(STRINGS
                ${openexr_config_file}
                TMP
                REGEX "#define OPENEXR_VERSION_MINOR.*$")
            string(REGEX MATCHALL "[0-9]" OPENEXR_MINOR_VERSION ${TMP})
        endif()

        foreach(OPENEXR_LIB
            IlmImf
            IlmImfUtil
            Half
            Iex
            IexMath
            Imath
            IlmThread)

            # OpenEXR libraries may be suffixed with the version number, so we search
            # using both versioned and unversioned names.
            find_library(OPENEXR_${OPENEXR_LIB}_LIBRARY_RELEASE
                NAMES
                    ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}
                    ${OPENEXR_LIB}

                PATHS ${VCPKG_INSTALL_ROOT}/lib NO_DEFAULT_PATH
            )
            #mark_as_advanced(OPENEXR_${OPENEXR_LIB}_LIBRARY)

            if(OPENEXR_${OPENEXR_LIB}_LIBRARY_RELEASE)
                list(APPEND OPENEXR_LIBRARIES ${OPENEXR_${OPENEXR_LIB}_LIBRARY_RELEASE})
            endif()

            # OpenEXR libraries may be suffixed with the version number, so we search
            # using both versioned and unversioned names.
            find_library(OPENEXR_${OPENEXR_LIB}_LIBRARY_DEBUG
                NAMES
                    ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_d
                    ${OPENEXR_LIB}_d

                PATHS ${VCPKG_INSTALL_ROOT}/debug/lib NO_DEFAULT_PATH
            )
            #mark_as_advanced(OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY)

            select_library_configurations(OPENEXR_${OPENEXR_LIB})

        endforeach(OPENEXR_LIB)

        target_link_libraries(${TARGET_NAME} ${OPENEXR_LIBRARIES})
        if (WIN32)
            target_compile_definitions(${TARGET_NAME} PUBLIC OPENEXR_DLL)
        endif(WIN32)
    endif()
endmacro()
