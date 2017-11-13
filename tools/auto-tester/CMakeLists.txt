set(TARGET_NAME auto-tester)
project(${TARGET_NAME})

# Automatically run UIC and MOC.  This replaces the older WRAP macros
SET(CMAKE_AUTOUIC ON) 
SET(CMAKE_AUTOMOC ON) 

setup_hifi_project(Widgets)
link_hifi_libraries()

set_property(TARGET auto-tester PROPERTY WIN32_EXECUTABLE true)

# This is preferred to GLOB_RECURSE, as per CMake documentation
set(SOURCES src/main.cpp src/Test.cpp src/ui/autoTester.cpp src/ui/mismatchWindow.cpp)
set(MOC_HEADERS src/autoTester.h src/mismatchWindow.h)
set(UIS src/ui/autoTester.ui src/ui/mismatchWindow.ui)

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