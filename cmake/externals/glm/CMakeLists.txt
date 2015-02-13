include(ExternalProject)
ExternalProject_Add(
  glm
  URL https://github.com/g-truc/glm/archive/0.9.5.4.zip
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON
)
ExternalProject_Get_Property(glm source_dir)
set(GLM_INCLUDE_DIRS ${source_dir}/glm)