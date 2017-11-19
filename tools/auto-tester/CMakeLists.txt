set(TARGET_NAME auto-tester)
project(${TARGET_NAME})

# Automatically run UIC and MOC.  This replaces the older WRAP macros
SET(CMAKE_AUTOUIC ON) 
SET(CMAKE_AUTOMOC ON) 

setup_hifi_project(Core Widgets)

# Qt includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})

# ITK includes
set (ITK_DIR ENV{ITK_DIR})
find_package (ITK REQUIRED)
include (${ITK_USE_FILE})

# Find all sources files
file (GLOB_RECURSE SOURCES src/*.cpp)
file (GLOB_RECURSE HEADERS src/*.h)
file (GLOB_RECURSE UIS src/ui/*.ui)

if (WIN32)
    # Do not show Console
    set_property(TARGET auto-tester PROPERTY WIN32_EXECUTABLE true)
endif()

add_executable(PROJECT_NAME ${SOURCES} ${HEADERS} ${UIS})
 
set(QT_LIBRARIES Qt5::Core Qt5::Widgets)
target_link_libraries(${TARGET_NAME} ${QT_LIBRARIES} ${ITK_LIBRARIES})

# Copy required dll's.
add_custom_command(TARGET auto-tester POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Core> $<TARGET_FILE_DIR:auto-tester>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Gui> $<TARGET_FILE_DIR:auto-tester>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Widgets> $<TARGET_FILE_DIR:auto-tester>
)
