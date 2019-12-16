include(ExternalProject)

set(EXTERNAL_NAME neuron)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

set(NEURON_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/neuron_datareader_b.12.2.zip")
set(NEURON_URL_MD5 "84273ad2200bf86a9279d1f412a822ca")

ExternalProject_Add(${EXTERNAL_NAME}
                    URL ${NEURON_URL}
                    URL_MD5 ${NEURON_URL_MD5}
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ""
                    INSTALL_COMMAND ""
                    LOG_DOWNLOAD 1)

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

# set include dir
if(WIN32)
    set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS "${SOURCE_DIR}/NeuronDataReader_Windows/include" CACHE STRING INTERNAL)
elseif(APPLE)
    set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS "${SOURCE_DIR}/NeuronDataReader_Mac/include" CACHE STRING INTERNAL)
else()
    # Unsupported
endif()

if(WIN32)

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(ARCH_DIR "x64")
    else()
        set(ARCH_DIR "x86")
    endif()

    set(${EXTERNAL_NAME_UPPER}_LIB_PATH "${SOURCE_DIR}/NeuronDataReader_Windows/lib/${ARCH_DIR}")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE "${${EXTERNAL_NAME_UPPER}_LIB_PATH}/NeuronDataReader.lib" CACHE STRING INTERNAL)
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES "${${EXTERNAL_NAME_UPPER}_LIB_PATH}/NeuronDataReader.lib" CACHE STRING INTERNAL)

    add_paths_to_fixup_libs("${${EXTERNAL_NAME_UPPER}_LIB_PATH}")

elseif(APPLE)

    set(${EXTERNAL_NAME_UPPER}_LIB_PATH "${SOURCE_DIR}/NeuronDataReader_Mac/dylib")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE "${${EXTERNAL_NAME_UPPER}_LIB_PATH}/NeuronDataReader.dylib" CACHE STRING INTERNAL)
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES "${${EXTERNAL_NAME_UPPER}_LIB_PATH}/NeuronDataReader.dylib" CACHE STRING INTERNAL)

    add_paths_to_fixup_libs("${${EXTERNAL_NAME_UPPER}_LIB_PATH}")

else()
    # UNSUPPORTED
endif()

