include(ExternalProject)
ExternalProject_Add(
  glm
  PREFIX glm
  URL https://github.com/g-truc/glm/archive/0.9.5.4.zip
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  LOG_DOWNLOAD ON
)
ExternalProject_Get_Property(glm install_dir)

export(TARGETS glm FILE ${CMAKE_BINARY_DIR}/glm-config.cmake)