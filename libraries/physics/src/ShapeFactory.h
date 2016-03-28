//
//  ShapeFactory.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.12.01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeFactory_h
#define hifi_ShapeFactory_h

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <ShapeInfo.h>

// translates between ShapeInfo and btShape

namespace ShapeFactory {
    btConvexHullShape* createConvexHull(const QVector<glm::vec3>& points);
    btCollisionShape* createShapeFromInfo(const ShapeInfo& info);
    void deleteShape(btCollisionShape* shape);
};

#endif // hifi_ShapeFactory_h
