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

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <ShapeInfo.h>

#include "DoubleHashKey.h"

// translates between ShapeInfo and btShape

namespace ShapeInfoUtil {
    void collectInfoFromShape(const btCollisionShape* shape, ShapeInfo& info);

    btCollisionShape* createShapeFromInfo(const ShapeInfo& info);

    DoubleHashKey computeHash(const ShapeInfo& info);

    // TODO? just use bullet shape types everywhere?
    int toBulletShapeType(int shapeInfoType);
    int fromBulletShapeType(int bulletShapeType);
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_ShapeInfoUtil_h
