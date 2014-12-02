//
//  ShapeInfoUtil.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.12.01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShapeInfoUtil.h"

#ifdef USE_BULLET_PHYSICS

void ShapeInfoUtil::collectInfoFromShape(const btCollisionShape* shape, ShapeInfo& info) {
    info._data.clear();
    if (shape) {
        info._type = (unsigned int)(shape->getShapeType());
        switch(_type) {
            case BOX_SHAPE_PROXYTYPE:
            {
                const btBoxShape* boxShape = static_cast<const btBoxShape*>(shape);
                info._data.push_back(boxShape->getHalfExtentsWithMargin());
            }
            break;
            case SPHERE_SHAPE_PROXYTYPE:
            {
                const btSphereShape* sphereShape = static_cast<const btSphereShape*>(shape);
                info._data.push_back(btVector3(0.0f, 0.0f, sphereShape->getRadius()));
            }
            break;
            case CYLINDER_SHAPE_PROXYTYPE:
            {
                const btCylinderShape* cylinderShape = static_cast<const btCylinderShape*>(shape);
                info._data.push_back(cylinderShape->getHalfExtentsWithMargin());
            }
            break;
            case CAPSULE_SHAPE_PROXYTYPE:
            {
                const btCapsuleShape* capsuleShape = static_cast<const btCapsuleShape*>(shape);
                info._data.push_back(btVector3(capsuleShape->getRadius(), capsuleShape->getHalfHeight(), 0.0f));
                // NOTE: we only support capsules with axis along yAxis
            }
            break;
            default:
                info._type = INVALID_SHAPE_PROXYTYPE;
            break;
        }
    } else {
        info._type = INVALID_SHAPE_PROXYTYPE;
    }
}

btCollisionShape* ShapeInfoUtil::createShape(const ShapeInfo& info) {
    btCollisionShape* shape = NULL;
    int numData = info._data.size();
    switch(info._type) {
        case BOX_SHAPE_PROXYTYPE: {
            if (numData > 0) {
                btVector3 halfExtents = info._data[0];
                shape = new btBoxShape(halfExtents);
            }
        }
        break;
        case SPHERE_SHAPE_PROXYTYPE: {
            if (numData > 0) {
                float radius = info._data[0].getZ();
                shape = new btSphereShape(radius);
            }
        }
        break;
        case CYLINDER_SHAPE_PROXYTYPE: {
            if (numData > 0) {
                btVector3 halfExtents = info._data[0];
                // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
                // halfExtents = btVector3(radius, halfHeight, unused)
                shape = new btCylinderShape(halfExtents);
            }
        }
        break;
        case CAPSULE_SHAPE_PROXYTYPE: {
            if (numData > 0) {
                float radius = info._data[0].getX();
                float height = 2.0f * info._data[0].getY();
                shape = new btCapsuleShape(radius, height);
            }
        }
        break;
    }
    return shape;
}

#endif // USE_BULLET_PHYSICS
