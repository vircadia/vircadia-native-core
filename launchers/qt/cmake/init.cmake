
# Hide automoc folders (for IDEs)
set(AUTOGEN_TARGETS_FOLDER "hidden/generated")
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# CMAKE_CURRENT_SOURCE_DIR is the parent folder here
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

file(GLOB LAUNCHER_CUSTOM_MACROS "cmake/macros/*.cmake")
foreach(CUSTOM_MACRO ${LAUNCHER_CUSTOM_MACROS})
  include(${CUSTOM_MACRO})
endforeach()
unset(LAUNCHER_CUSTOM_MACROS)
