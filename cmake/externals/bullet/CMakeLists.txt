set(EXTERNAL_NAME bullet)

if (ANDROID)
  set(PLATFORM_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
elseif (WIN32)
  set(PLATFORM_CMAKE_ARGS "-DUSE_MSVC_RUNTIME_LIBRARY_DLL=1")
endif ()

if (WIN32)
  set(DOWNLOAD_URL https://bullet.googlecode.com/files/bullet-2.82-r2704.zip)
  set(DOWNLOAD_MD5 f5e8914fc9064ad32e0d62d19d33d977)
else ()
  set(DOWNLOAD_URL http://bullet.googlecode.com/files/bullet-2.82-r2704.tgz)
  set(DOWNLOAD_MD5 70b3c8d202dee91a0854b4cbc88173e8)
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL ${DOWNLOAD_URL}
  URL_MD5 ${DOWNLOAD_MD5}
  CMAKE_ARGS ${PLATFORM_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DINSTALL_LIBS=1 -DBUILD_DEMOS=0 -DBUILD_SHARED_LIBS=1 -DUSE_GLM=0
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INSTALL_DIR}/include/bullet CACHE TYPE "Path to bullet include directory")

set(BULLET_LIB_DIR "${INSTALL_DIR}/lib")

if (APPLE OR UNIX OR ANDROID)
  if (APPLE)
    set(SHARED_LIB_EXT "dylib")
  else ()
    set(SHARED_LIB_EXT "so")
  endif ()
  
  set(${EXTERNAL_NAME_UPPER}_DYNAMICS_LIBRARY_RELEASE ${BULLET_LIB_DIR}/libBulletDynamics.${SHARED_LIB_EXT} CACHE TYPE "Bullet dynamics release library location")
  set(${EXTERNAL_NAME_UPPER}_DYNAMICS_LIBRARY_DEBUG NOTFOUND CACHE TYPE "Bullet dynamics debug library location")

  set(${EXTERNAL_NAME_UPPER}_COLLISION_LIBRARY_RELEASE ${BULLET_LIB_DIR}/libBulletCollision.${SHARED_LIB_EXT} CACHE TYPE "Bullet collision release library location")
  set(${EXTERNAL_NAME_UPPER}_COLLISION_LIBRARY_DEBUG NOTFOUND CACHE TYPE "Bullet collision debug library location")

  set(${EXTERNAL_NAME_UPPER}_MATH_LIBRARY_RELEASE ${BULLET_LIB_DIR}/libLinearMath.${SHARED_LIB_EXT} CACHE TYPE "Bullet math release library location")
  set(${EXTERNAL_NAME_UPPER}_MATH_LIBRARY_DEBUG NOTFOUND CACHE TYPE "Bullet math debug library location")

  set(${EXTERNAL_NAME_UPPER}_SOFTBODY_LIBRARY_RELEASE ${BULLET_LIB_DIR}/libBulletSoftBody.${SHARED_LIB_EXT} CACHE TYPE "Bullet softbody release library location")
  set(${EXTERNAL_NAME_UPPER}_SOFTBODY_LIBRARY_DEBUG NOTFOUND CACHE TYPE "Bullet softbody debug library location")
elseif (WIN32)
  
endif ()