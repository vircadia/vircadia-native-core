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


// find the average point on a convex shape
glm::vec3 findCenter(const QVector<glm::vec3>& points) {
    glm::vec3 result = glm::vec3(0);
    for (int i = 0; i < points.size(); i++) {
        result += points[i];
    }
    return result * (1.0f / points.size());
}


// bullet puts "margins" around all the collision shapes.  This can cause shapes will hulls
// to float a bit above what they are sitting on, etc.  One option is to call:
//
//    compound->setMargin(0.01);
//
// to reduce the size of the margin, but this has some consequences for the
// performance and stability of the simulation.  Instead, we clench in all the points of
// the hull by the margin.  These clenched points + bullets margin will but the actual
// collision hull fairly close to the visual edge of the object.
QVector<glm::vec3> shrinkByMargin(const QVector<glm::vec3>& points, const glm::vec3 center, float margin) {
    QVector<glm::vec3> result(points.size());
    for (int i = 0; i < points.size(); i++) {
        glm::vec3 pVec = points[ i ] - center;
        glm::vec3 pVecNorm = glm::normalize(pVec);
        result[ i ] = center + pVec - (pVecNorm * margin);
    }
    return result;
}


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
                glm::vec3 center = findCenter(points[0]);
                QVector<glm::vec3> shrunken = shrinkByMargin(points[0], center, hull->getMargin());
                foreach (glm::vec3 point, shrunken) {
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
                    glm::vec3 center = findCenter(points[0]);
                    QVector<glm::vec3> shrunken = shrinkByMargin(hullPoints, center, hull->getMargin());
                    foreach (glm::vec3 point, shrunken) {
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
