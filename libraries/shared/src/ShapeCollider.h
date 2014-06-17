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

#include "CapsuleShape.h"
#include "CollisionInfo.h"
#include "ListShape.h"
#include "PlaneShape.h"
#include "SharedUtil.h" 
#include "SphereShape.h"

namespace ShapeCollider {

    /// \param shapeA pointer to first shape (cannot be NULL)
    /// \param shapeB pointer to second shape (cannot be NULL)
    /// \param collisions[out] collision details
    /// \return true if shapes collide
    bool collideShapes(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);

    /// \param shapesA list of shapes
    /// \param shapeB list of shapes
    /// \param collisions[out] average collision details
    /// \return true if any shapes collide
    bool collideShapesCoarse(const QVector<const Shape*>& shapesA, const QVector<const Shape*>& shapesB, CollisionInfo& collision);

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
    bool sphereSphere(const SphereShape* sphereA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereCapsule(const SphereShape* sphereA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool spherePlane(const SphereShape* sphereA, const PlaneShape* planeB, CollisionList& collisions);
    
    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleSphere(const CapsuleShape* capsuleA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleCapsule(const CapsuleShape* capsuleA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsulePlane(const CapsuleShape* capsuleA, const PlaneShape* planeB, CollisionList& collisions);
    
    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeSphere(const PlaneShape* planeA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeCapsule(const PlaneShape* planeA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planePlane(const PlaneShape* planeA, const PlaneShape* planeB, CollisionList& collisions);
    
    /// \param sphereA pointer to first shape (cannot be NULL)
    /// \param listB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereList(const SphereShape* sphereA, const ListShape* listB, CollisionList& collisions);

    /// \param capuleA pointer to first shape (cannot be NULL)
    /// \param listB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleList(const CapsuleShape* capsuleA, const ListShape* listB, CollisionList& collisions);

    /// \param planeA pointer to first shape (cannot be NULL)
    /// \param listB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeList(const PlaneShape* planeA, const ListShape* listB, CollisionList& collisions);
    
    /// \param listA pointer to first shape (cannot be NULL)
    /// \param sphereB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listSphere(const ListShape* listA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param listA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listCapsule(const ListShape* listA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param listA pointer to first shape (cannot be NULL)
    /// \param planeB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listPlane(const ListShape* listA, const PlaneShape* planeB, CollisionList& collisions);
    
    /// \param listA pointer to first shape (cannot be NULL)
    /// \param capsuleB pointer to second shape (cannot be NULL)
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listList(const ListShape* listA, const ListShape* listB, CollisionList& collisions);

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

    /// \param shapeA pointer to shape (cannot be NULL)
    /// \param startPoint beginning of ray
    /// \param direction direction of ray
    /// \param distance[out] distance to intersection of shape and ray
    /// \return true if ray hits shapeA
    bool findRayIntersectionWithShape(const Shape* shapeA, const glm::vec3& startPoint, const glm::vec3& direction, float& distance);

}   // namespace ShapeCollider

#endif // hifi_ShapeCollider_h
