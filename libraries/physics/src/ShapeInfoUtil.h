//
//  ShapeInfoUtil.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.12.01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeInfoUtil_h
#define hifi_ShapeInfoUtil_h

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <ShapeInfo.h>

// translates between ShapeInfo and btShape

// TODO: rename this to ShapeFactory
namespace ShapeInfoUtil {

    btCollisionShape* createShapeFromInfo(const ShapeInfo& info);
};

#endif // hifi_ShapeInfoUtil_h
