//
//  ShapeCollider.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 02/20/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeCollider_h
#define hifi_ShapeCollider_h

#include <QVector>

#include "CollisionInfo.h"
#include "SharedUtil.h" 

class Shape;
class SphereShape;
class CapsuleShape;

namespace ShapeCollider {

    /// MUST CALL this FIRST before using the ShapeCollider
    void initDispatchTable();

    /// \param shapeA pointer to first shape (cannot be NULL)
    /// \param shapeB pointer to second shape (cannot be NULL)
    /// \param collisions[out] collision details
    /// \return true if shapes collide
    bool collideShapes(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);

    bool collideShapeWithShapes(const Shape* shapeA, const QVector<Shape*>& shapes, int startIndex, CollisionList& collisions);
    bool collideShapesWithShapes(const QVector<Shape*>& shapesA, const QVector<Shape*>& shapesB, CollisionList& collisions);

    /// \param shapeA a pointer to a shape (cannot be NULL)
    /// \param cubeCenter center of cube
    /// \param cubeSide lenght of side of cube
    /// \param collisions[out] average collision details
    /// \return true if shapeA collides with axis aligned cube
    bool collideShapeWithAACubeLegacy(const Shape* shapeA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions);

    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereVsSphere(const Shape* sphereA, const Shape* sphereB, CollisionList& collisions);

    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereVsCapsule(const Shape* sphereA, const Shape* capsuleB, CollisionList& collisions);

    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereVsPlane(const Shape* sphereA, const Shape* planeB, CollisionList& collisions);
    
    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleVsSphere(const Shape* capsuleA, const Shape* sphereB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleVsCapsule(const Shape* capsuleA, const Shape* capsuleB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleVsPlane(const Shape* capsuleA, const Shape* planeB, CollisionList& collisions);
    
    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeVsSphere(const Shape* planeA, const Shape* sphereB, CollisionList& collisions);

    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeVsCapsule(const Shape* planeA, const Shape* capsuleB, CollisionList& collisions);

    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeVsPlane(const Shape* planeA, const Shape* planeB, CollisionList& collisions);

    /// helper function for *VsAACube() methods
    /// \param sphereCenter center of sphere
    /// \param sphereRadius radius of sphere
    /// \param cubeCenter center of AACube
    /// \param cubeSide scale of cube
    /// \param[out] collisions where to append collision details
    /// \return valid pointer to CollisionInfo if sphere and cube overlap or NULL if not
    CollisionInfo* sphereVsAACubeHelper(const glm::vec3& sphereCenter, float sphereRadius, 
            const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions);

    bool sphereVsAACube(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);
    bool capsuleVsAACube(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);
    bool aaCubeVsSphere(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);
    bool aaCubeVsCapsule(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);
    bool aaCubeVsAACube(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);
    
    /// \param shapeA pointer to first shape (cannot be NULL)
    /// \param listB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool shapeVsList(const Shape* shapeA, const Shape* listB, CollisionList& collisions);

    /// \param listA pointer to first shape (cannot be NULL)
    /// \param shapeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listVsShape(const Shape* listA, const Shape* shapeB, CollisionList& collisions);
    
    /// \param listA pointer to first shape (cannot be NULL)
    /// \param listB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listVsList(const Shape* listA, const Shape* listB, CollisionList& collisions);

    /// \param sphereA pointer to sphere (cannot be NULL)
    /// \param cubeCenter center of cube
    /// \param cubeSide lenght of side of cube
    /// \param[out] collisions where to append collision details
    /// \return true if sphereA collides with axis aligned cube
    bool sphereVsAACubeLegacy(const SphereShape* sphereA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions);

    /// \param capsuleA pointer to capsule (cannot be NULL)
    /// \param cubeCenter center of cube
    /// \param cubeSide lenght of side of cube
    /// \param[out] collisions where to append collision details
    /// \return true if capsuleA collides with axis aligned cube
    bool capsuleVsAACubeLegacy(const CapsuleShape* capsuleA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions);

    /// \param shapes list of pointers to shapes (shape pointers may be NULL)
    /// \param startPoint beginning of ray
    /// \param direction direction of ray
    /// \param minDistance[out] shortest distance to intersection of ray with a shapes
    /// \return true if ray hits any shape in shapes
    bool findRayIntersectionWithShapes(const QVector<Shape*> shapes, const glm::vec3& startPoint, const glm::vec3& direction, float& minDistance);

}   // namespace ShapeCollider

#endif // hifi_ShapeCollider_h
