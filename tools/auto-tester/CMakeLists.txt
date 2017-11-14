set(TARGET_NAME auto-tester)
project(${TARGET_NAME})

# Automatically run UIC and MOC.  This replaces the older WRAP macros
SET(CMAKE_AUTOUIC ON) 
SET(CMAKE_AUTOMOC ON) 

setup_hifi_project(Widgets)
link_hifi_libraries()

set_property(TARGET auto-tester PROPERTY WIN32_EXECUTABLE true)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

# Find all sources files
file (GLOB_RECURSE SOURCES src/*.cpp)
file (GLOB_RECURSE MOC_HEADERS src/*.h)
file (GLOB_RECURSE UIS src/ui/*.ui)

if (WIN32)
  add_executable(PROJECT_NAME WIN32 ${SOURCES} ${MOC_SRCS} ${UI_HEADERS})
else()
  add_executable(PROJECT_NAME ${SOURCES} ${MOC_SRCS} ${RES_SOURCES} ${UI_HEADERS})
endif()
 
target_link_libraries(PROJECT_NAME ${QT_LIBRARIES})

# Copy required dll's.
# Note that the two ImageMagick files are copied twice.  This is to allow the tester to run from VS as well as 
# directly from the executable.
add_custom_command(
    TARGET auto-tester
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Core> $<TARGET_FILE_DIR:auto-tester>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Gui> $<TARGET_FILE_DIR:auto-tester>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Widgets> $<TARGET_FILE_DIR:auto-tester>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ImageMagick/magick.exe $<TARGET_FILE_DIR:auto-tester>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ImageMagick/magic.xml $<TARGET_FILE_DIR:auto-tester>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ImageMagick/magick.exe $<TARGET_FILE_DIR:auto-tester>/..
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ImageMagick/magic.xml $<TARGET_FILE_DIR:auto-tester>/..
)