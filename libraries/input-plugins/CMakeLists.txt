set(TARGET_NAME input-plugins)
setup_hifi_library()
link_hifi_libraries(shared plugins gpu render-utils)

GroupSources("src/input-plugins")

if (WIN32)
    add_dependency_external_projects(OpenVR)
    find_package(OpenVR REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${OPENVR_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARIES})
endif()

#add_dependency_external_projects(Sixense)
#find_package(Sixense REQUIRED)
#target_include_directories(${TARGET_NAME} PRIVATE ${SIXENSE_INCLUDE_DIRS})
#target_link_libraries(${TARGET_NAME} ${SIXENSE_LIBRARIES})

# set a default root dir for each of our optional externals if it was not passed
set(OPTIONAL_EXTERNALS "SDL2" "Sixense")
foreach(EXTERNAL ${OPTIONAL_EXTERNALS})
  string(TOUPPER ${EXTERNAL} ${EXTERNAL}_UPPERCASE)
  if (NOT ${${EXTERNAL}_UPPERCASE}_ROOT_DIR)
    string(TOLOWER ${EXTERNAL} ${EXTERNAL}_LOWERCASE)
    set(${${EXTERNAL}_UPPERCASE}_ROOT_DIR "${CMAKE_SOURCE_DIR}/interface/external/${${EXTERNAL}_LOWERCASE}")
  endif ()
endforeach()

# perform standard include and linking for found externals
foreach(EXTERNAL ${OPTIONAL_EXTERNALS})
  
  if (${${EXTERNAL}_UPPERCASE}_REQUIRED)
    find_package(${EXTERNAL} REQUIRED)
  else ()
    find_package(${EXTERNAL})
  endif ()
  
  if (${${EXTERNAL}_UPPERCASE}_FOUND AND NOT DISABLE_${${EXTERNAL}_UPPERCASE})
    add_definitions(-DHAVE_${${EXTERNAL}_UPPERCASE})
    
    # include the library directories (ignoring warnings)
    if (NOT ${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS)
      set(${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIR})
    endif ()
    
    include_directories(SYSTEM ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS})
    
    # perform the system include hack for OS X to ignore warnings
    if (APPLE)
      foreach(EXTERNAL_INCLUDE_DIR  ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS})
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${EXTERNAL_INCLUDE_DIR}")
      endforeach()
    endif ()
    
    if (NOT ${${EXTERNAL}_UPPERCASE}_LIBRARIES)
      set(${${EXTERNAL}_UPPERCASE}_LIBRARIES ${${${EXTERNAL}_UPPERCASE}_LIBRARY})
    endif ()
    
    if (NOT APPLE OR NOT ${${EXTERNAL}_UPPERCASE} MATCHES "SIXENSE")
      target_link_libraries(${TARGET_NAME} ${${${EXTERNAL}_UPPERCASE}_LIBRARIES})
    elseif (APPLE AND NOT INSTALLER_BUILD)
      add_definitions(-DSIXENSE_LIB_FILENAME=\"${${${EXTERNAL}_UPPERCASE}_LIBRARY_RELEASE}\")
    endif ()
  endif ()
endforeach()
