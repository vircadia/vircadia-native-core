set(TARGET_NAME auto-tester)
project(${TARGET_NAME})

setup_hifi_project(Widgets)
link_hifi_libraries()

set ( SOURCES
    src/main.cpp
    src/Test.cpp
    src/ui/AutoTester.cpp
    src/ui/MismatchWindow.cpp
)

set ( MOC_HEADERS
    src/ui/AutoTester.h
    src/ui/MismatchWindow.h
)

set ( UIS
    ui/AutoTester.ui
    ui/MismatchWindow.ui
)

set ( RESOURCES
    src/AutoTester.qrc
)

QT5_ADD_RESOURCES( RES_SOURCES ${RESOURCES} )
QT5_WRAP_UI( UI_HEADERS ${UIS} )
QT5_WRAP_CPP( MOC_SRCS ${MOC_HEADERS} )

add_executable( PROJECT_NAME ${SOURCES} ${MOC_SRCS} ${RES_SOURCES} ${UI_HEADERS} )

target_link_libraries( PROJECT_NAME ${QT_LIBRARIES} )