if (WIN32)
  cmake_policy(SET CMP0020 NEW)
endif (WIN32)

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif ()

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 OLD)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
# Hide automoc folders (for IDEs)
set(AUTOGEN_TARGETS_FOLDER "hidden/generated")
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (CMAKE_BUILD_TYPE)
  string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_CMAKE_BUILD_TYPE)
else ()
  set(UPPER_CMAKE_BUILD_TYPE DEBUG)
endif ()

# CMAKE_CURRENT_SOURCE_DIR is the parent folder here
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

set(HF_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(MACRO_DIR "${HF_CMAKE_DIR}/macros")
set(EXTERNAL_PROJECT_DIR "${HF_CMAKE_DIR}/externals")

file(GLOB HIFI_CUSTOM_MACROS "cmake/macros/*.cmake")
foreach(CUSTOM_MACRO ${HIFI_CUSTOM_MACROS})
  include(${CUSTOM_MACRO})
endforeach()

if (ANDROID)
  file(GLOB ANDROID_CUSTOM_MACROS "cmake/android/*.cmake")
  foreach(CUSTOM_MACRO ${ANDROID_CUSTOM_MACROS})
    include(${CUSTOM_MACRO})
  endforeach()
endif ()
