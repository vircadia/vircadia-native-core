set (TARGET_NAME auto-tester)
project(${TARGET_NAME})

# Automatically run UIC and MOC.  This replaces the older WRAP macros
SET (CMAKE_AUTOUIC ON) 
SET (CMAKE_AUTOMOC ON) 

setup_hifi_project (Core Widgets Network Xml)
link_hifi_libraries ()

# FIX: Qt was built with -reduce-relocations
if (Qt5_POSITION_INDEPENDENT_CODE)
    SET (CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Qt includes
include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${Qt5Core_INCLUDE_DIRS})
include_directories (${Qt5Widgets_INCLUDE_DIRS})

set (QT_LIBRARIES Qt5::Core Qt5::Widgets QT::Gui Qt5::Xml)

if (WIN32)
    # Do not show Console
    set_property (TARGET auto-tester PROPERTY WIN32_EXECUTABLE true)
endif()

target_zlib()
add_dependency_external_projects (quazip)
find_package (QuaZip REQUIRED)
target_include_directories( ${TARGET_NAME} SYSTEM PUBLIC ${QUAZIP_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${QUAZIP_LIBRARIES})
 
package_libraries_for_deployment()

if (WIN32)
    add_paths_to_fixup_libs (${QUAZIP_DLL_PATH})

    find_program(WINDEPLOYQT_COMMAND windeployqt PATHS ${QT_DIR}/bin NO_DEFAULT_PATH)
    
    if (NOT WINDEPLOYQT_COMMAND)
      message(FATAL_ERROR "Could not find windeployqt at ${QT_DIR}/bin. windeployqt is required.")
    endif ()

    # add a post-build command to call windeployqt to copy Qt plugins
    add_custom_command(
      TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND CMD /C "SET PATH=%PATH%;${QT_DIR}/bin && ${WINDEPLOYQT_COMMAND} ${EXTRA_DEPLOY_OPTIONS} $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>,$<CONFIG:RelWithDebInfo>>:--release> \"$<TARGET_FILE:${TARGET_NAME}>\""
    )
endif ()