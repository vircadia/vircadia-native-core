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
//#include "CapsuleShape.h"
//#include "ListShape.h"
//#include "PlaneShape.h"
//#include "SphereShape.h"

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
    bool collideShapesOld(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);

    /// \param shapesA list of shapes
    /// \param shapeB list of shapes
    /// \param collisions[out] average collision details
    /// \return true if any shapes collide
    bool collideShapesCoarse(const QVector<const Shape*>& shapesA, const QVector<const Shape*>& shapesB, CollisionInfo& collision);

    bool collideShapeWithShapes(const Shape* shapeA, const QVector<Shape*>& shapes, int startIndex, CollisionList& collisions);
    bool collideShapesWithShapes(const QVector<Shape*>& shapesA, const QVector<Shape*>& shapesB, CollisionList& collisions);

    /// \param shapeA a pointer to a shape (cannot be NULL)
    /// \param cubeCenter center of cube
    /// \param cubeSide lenght of side of cube
    /// \param collisions[out] average collision details
    /// \return true if shapeA collides with axis aligned cube
    bool collideShapeWithAACube(const Shape* shapeA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions);

    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereSphere(const Shape* sphereA, const Shape* sphereB, CollisionList& collisions);

    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereCapsule(const Shape* sphereA, const Shape* capsuleB, CollisionList& collisions);

    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool spherePlane(const Shape* sphereA, const Shape* planeB, CollisionList& collisions);
    
    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleSphere(const Shape* capsuleA, const Shape* sphereB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleCapsule(const Shape* capsuleA, const Shape* capsuleB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsulePlane(const Shape* capsuleA, const Shape* planeB, CollisionList& collisions);
    
    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeSphere(const Shape* planeA, const Shape* sphereB, CollisionList& collisions);

    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeCapsule(const Shape* planeA, const Shape* capsuleB, CollisionList& collisions);

    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planePlane(const Shape* planeA, const Shape* planeB, CollisionList& collisions);
    
    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param listB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereList(const Shape* sphereA, const Shape* listB, CollisionList& collisions);

    /// \param capuleA pointer to first shape (cannot be NULL)
    /// \param listB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleList(const Shape* capsuleA, const Shape* listB, CollisionList& collisions);

    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param listB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeList(const Shape* planeA, const Shape* listB, CollisionList& collisions);
    
    /// \param listA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listSphere(const Shape* listA, const Shape* sphereB, CollisionList& collisions);

    /// \param listA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listCapsule(const Shape* listA, const Shape* capsuleB, CollisionList& collisions);

    /// \param listA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listPlane(const Shape* listA, const Shape* planeB, CollisionList& collisions);
    
    /// \param listA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listList(const Shape* listA, const Shape* listB, CollisionList& collisions);

    /// \param sphereA pointer to sphere (cannot be NULL)
    /// \param cubeCenter center of cube
    /// \param cubeSide lenght of side of cube
    /// \param[out] collisions where to append collision details
    /// \return true if sphereA collides with axis aligned cube
    bool sphereAACube(const SphereShape* sphereA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions);

    /// \param capsuleA pointer to capsule (cannot be NULL)
    /// \param cubeCenter center of cube
    /// \param cubeSide lenght of side of cube
    /// \param[out] collisions where to append collision details
    /// \return true if capsuleA collides with axis aligned cube
    bool capsuleAACube(const CapsuleShape* capsuleA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions);

    /// \param shapes list of pointers to shapes (shape pointers may be NULL)
    /// \param startPoint beginning of ray
    /// \param direction direction of ray
    /// \param minDistance[out] shortest distance to intersection of ray with a shapes
    /// \return true if ray hits any shape in shapes
    bool findRayIntersectionWithShapes(const QVector<Shape*> shapes, const glm::vec3& startPoint, const glm::vec3& direction, float& minDistance);

}   // namespace ShapeCollider

#endif // hifi_ShapeCollider_h
