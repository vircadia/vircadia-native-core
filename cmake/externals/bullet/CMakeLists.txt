set(EXTERNAL_NAME bullet)

if (WIN32)
  set(PLATFORM_CMAKE_ARGS "-DUSE_MSVC_RUNTIME_LIBRARY_DLL=1")
else ()
  set(PLATFORM_CMAKE_ARGS "-DBUILD_SHARED_LIBS=1")
  
  if (ANDROID)
    list(APPEND PLATFORM_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
  elseif (APPLE)
    list(APPEND PLATFORM_CMAKE_ARGS "-DCMAKE_INSTALL_NAME_DIR=<INSTALL_DIR>/lib")
  endif()
endif ()

include(ExternalProject)

if (WIN32)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MSBUILD_CONFIGURATION Debug)
  else ()
    set(MSBUILD_CONFIGURATION Release)
  endif ()
  
  find_program(MSBUILD_COMMAND msbuild)
  
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL https://bullet.googlecode.com/files/bullet-2.82-r2704.zip
    URL_MD5 f5e8914fc9064ad32e0d62d19d33d977
    CMAKE_ARGS ${PLATFORM_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DBUILD_EXTRAS=0 -DINSTALL_LIBS=1 -DBUILD_DEMOS=0 -DUSE_GLUT=0
    BUILD_COMMAND ${MSBUILD_COMMAND} bullet.vcxproj /t:Build /p:Configuration=${MSBUILD_CONFIGURATION}
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
    BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  )  
else ()
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://bullet.googlecode.com/files/bullet-2.82-r2704.tgz
    URL_MD5 70b3c8d202dee91a0854b4cbc88173e8
    CMAKE_ARGS ${PLATFORM_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DBUILD_EXTRAS=0 -DINSTALL_LIBS=1 -DBUILD_DEMOS=0 -DUSE_GLUT=0
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
    BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  )  
endif ()

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

set(BULLET_LIB_DIR "${INSTALL_DIR}/lib")

if (APPLE OR UNIX OR ANDROID)
  if (APPLE)
    set(BULLET_LIB_EXT "dylib")
  else ()
    set(BULLET_LIB_EXT "so")
  endif ()
  
  set(LIB_PREFIX "lib")
elseif (WIN32)
  set(BULLET_LIB_EXT "lib")
endif ()

if (DEFINED BULLET_LIB_EXT)
  set(${EXTERNAL_NAME_UPPER}_DYNAMICS_LIBRARY_RELEASE ${BULLET_LIB_DIR}/${LIB_PREFIX}BulletDynamics.${BULLET_LIB_EXT} CACHE FILEPATH "Bullet dynamics release library location")
  set(${EXTERNAL_NAME_UPPER}_DYNAMICS_LIBRARY_DEBUG "" CACHE FILEPATH "Bullet dynamics debug library location")

  set(${EXTERNAL_NAME_UPPER}_COLLISION_LIBRARY_RELEASE ${BULLET_LIB_DIR}/${LIB_PREFIX}BulletCollision.${BULLET_LIB_EXT} CACHE FILEPATH "Bullet collision release library location")
  set(${EXTERNAL_NAME_UPPER}_COLLISION_LIBRARY_DEBUG "" CACHE FILEPATH "Bullet collision debug library location")

  set(${EXTERNAL_NAME_UPPER}_MATH_LIBRARY_RELEASE ${BULLET_LIB_DIR}/${LIB_PREFIX}LinearMath.${BULLET_LIB_EXT} CACHE FILEPATH "Bullet math release library location")
  set(${EXTERNAL_NAME_UPPER}_MATH_LIBRARY_DEBUG "" CACHE FILEPATH "Bullet math debug library location")

  set(${EXTERNAL_NAME_UPPER}_SOFTBODY_LIBRARY_RELEASE ${BULLET_LIB_DIR}/${LIB_PREFIX}BulletSoftBody.${BULLET_LIB_EXT} CACHE FILEPATH "Bullet softbody release library location")
  set(${EXTERNAL_NAME_UPPER}_SOFTBODY_LIBRARY_DEBUG "" CACHE FILEPATH "Bullet softbody debug library location")
endif ()

if (DEFINED ${EXTERNAL_NAME_UPPER}_DYNAMICS_LIBRARY_RELEASE)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INSTALL_DIR}/include/bullet CACHE PATH "Path to bullet include directory")
endif ()