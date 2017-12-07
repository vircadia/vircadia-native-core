//
//  ShapeFactory.h
//  libraries/physics/src
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

// The ShapeFactory assembles and correctly disassembles btCollisionShapes.

namespace ShapeFactory {
    const btCollisionShape* createShapeFromInfo(const ShapeInfo& info);
    void deleteShape(const btCollisionShape* shape);
};

#endif // hifi_ShapeFactory_h
