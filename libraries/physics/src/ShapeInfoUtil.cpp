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

#include <SharedUtil.h> // for MILLIMETERS_PER_METER

#include "ShapeInfoUtil.h"
#include "BulletUtil.h"


btCollisionShape* ShapeInfoUtil::createShapeFromInfo(const ShapeInfo& info) {
    btCollisionShape* shape = NULL;
    switch(info.getType()) {
        case SHAPE_TYPE_BOX: {
            shape = new btBoxShape(glmToBullet(info.getHalfExtents()));
        }
        break;
        case SHAPE_TYPE_SPHERE: {
            float radius = info.getHalfExtents().x;
            shape = new btSphereShape(radius);
        }
        break;
        case SHAPE_TYPE_CAPSULE_Y: {
            glm::vec3 halfExtents = info.getHalfExtents();
            float radius = halfExtents.x;
            float height = 2.0f * halfExtents.y;
            shape = new btCapsuleShape(radius, height);
        }
        break;
        case SHAPE_TYPE_COMPOUND: {
            const QVector<QVector<glm::vec3>>& points = info.getPoints();
            uint32_t numSubShapes = info.getNumSubShapes();
            if (numSubShapes == 1) {
                auto hull = new btConvexHullShape();
                const QVector<QVector<glm::vec3>>& points = info.getPoints();
                foreach (glm::vec3 point, points[0]) {
                    btVector3 btPoint(point[0], point[1], point[2]);
                    hull->addPoint(btPoint, false);
                }
                hull->recalcLocalAabb();
                shape = hull;
            } else {
                assert(numSubShapes > 1);
                auto compound = new btCompoundShape();
                btTransform trans;
                trans.setIdentity();
                foreach (QVector<glm::vec3> hullPoints, points) {
                    auto hull = new btConvexHullShape();
                    foreach (glm::vec3 point, hullPoints) {
                        btVector3 btPoint(point[0], point[1], point[2]);
                        hull->addPoint(btPoint, false);
                    }
                    hull->recalcLocalAabb();
                    compound->addChildShape (trans, hull);
                }
                shape = compound;
            }
        }
        break;
    }
    return shape;
}
