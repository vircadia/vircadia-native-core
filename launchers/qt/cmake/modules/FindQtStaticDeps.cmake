
set(qt_static_lib_dependices
  "qtpcre2"
  "qtlibpng"
  "qtfreetype"
  "Qt5AccessibilitySupport"
  "Qt5FbSupport"
  "Qt5OpenGLExtensions"
  "Qt5QuickTemplates2"
  "Qt5FontDatabaseSupport"
  "Qt5ThemeSupport"
  "Qt5EventDispatcherSupport")

if (WIN32)
elseif(APPLE)
  set(qt_static_lib_dependices
    ${qt_static_lib_dependices}
    "Qt5GraphicsSupport"
    "Qt5CglSupport"
    "Qt5ClipboardSupport")
endif()

set(LIBS_PREFIX "${_qt5Core_install_prefix}/lib/")
foreach (_qt_static_dep ${qt_static_lib_dependices})
  if (WIN32)
    set(lib_path "${LIBS_PREFIX}${_qt_static_dep}.lib")
  else()
    set(lib_path "${LIBS_PREFIX}lib${_qt_static_dep}.a")
  endif()
  set(QT_STATIC_LIBS ${QT_STATIC_LIBS} ${lib_path})
endforeach()

unset(qt_static_lib_dependices)
