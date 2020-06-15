macro(TARGET_PYTHON)
    if (NOT HIFI_PYTHON_EXEC)
        # Find the python interpreter
        if (CMAKE_VERSION VERSION_LESS 3.12)
            # this logic is deprecated in CMake after 3.12
            # FIXME eventually we should make 3.12 the min cmake verion and just use the Python3 find_package path
            set(Python_ADDITIONAL_VERSIONS 3)
            find_package(PythonInterp)
            set(HIFI_PYTHON_VERSION ${PYTHON_VERSION_STRING})
            set(HIFI_PYTHON_EXEC ${PYTHON_EXECUTABLE})
        else()
            # the new hotness
            find_package(Python3)
            set(HIFI_PYTHON_VERSION ${Python3_VERSION})
            set(HIFI_PYTHON_EXEC ${Python3_EXECUTABLE})
        endif()

        if ((NOT HIFI_PYTHON_EXEC) OR (HIFI_PYTHON_VERSION VERSION_LESS 3.5))
            message(FATAL_ERROR "Unable to locate Python interpreter 3.5 or higher")
        endif()
    endif()
    message("Using the Python interpreter located at: " ${HIFI_PYTHON_EXEC})
endmacro()
