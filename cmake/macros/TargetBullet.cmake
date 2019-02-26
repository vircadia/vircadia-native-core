# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_BULLET)
    if (ANDROID)
        set(INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/bullet)
        set(BULLET_INCLUDE_DIRS "${INSTALL_DIR}/include/bullet" CACHE TYPE INTERNAL)

        set(LIB_DIR ${INSTALL_DIR}/lib)
        list(APPEND BULLET_LIBRARIES ${LIB_DIR}/libBulletDynamics.a)
        list(APPEND BULLET_LIBRARIES ${LIB_DIR}/libBulletCollision.a)
        list(APPEND BULLET_LIBRARIES ${LIB_DIR}/libLinearMath.a)
        list(APPEND BULLET_LIBRARIES ${LIB_DIR}/libBulletSoftBody.a)
    else()
        find_package(Bullet REQUIRED)
   endif()
    # perform the system include hack for OS X to ignore warnings
    if (APPLE)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${BULLET_INCLUDE_DIRS}")
    else()
      target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${BULLET_INCLUDE_DIRS})
    endif()
    target_link_libraries(${TARGET_NAME} ${BULLET_LIBRARIES})
endmacro()


