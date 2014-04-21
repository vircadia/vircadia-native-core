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

#include "AABox.h"
#include "CapsuleShape.h"
#include "CollisionInfo.h"
#include "ListShape.h"
#include "PlaneShape.h"
#include "SharedUtil.h" 
#include "SphereShape.h"

namespace ShapeCollider {

    /// \param shapeA pointer to first shape
    /// \param shapeB pointer to second shape
    /// \param collisions[out] collision details
    /// \return true if shapes collide
    bool collideShapes(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);

    /// \param shapesA list of shapes
    /// \param shapeB list of shapes
    /// \param collisions[out] average collision details
    /// \return true if any shapes collide
    bool collideShapesCoarse(const QVector<const Shape*>& shapesA, const QVector<const Shape*>& shapesB, CollisionInfo& collision);

    /// \param shapeA a pointer to a shape
    /// \param boxB an axis aligned box
    /// \param collisions[out] average collision details
    /// \return true if shapeA collides with boxB
    bool collideShapeWithBox(const Shape* shapeA, const AABox& boxB, CollisionList& collisions);

    /// \param sphereA pointer to first shape
    /// \param sphereB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereSphere(const SphereShape* sphereA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param sphereA pointer to first shape
    /// \param capsuleB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereCapsule(const SphereShape* sphereA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param sphereA pointer to first shape
    /// \param planeB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool spherePlane(const SphereShape* sphereA, const PlaneShape* planeB, CollisionList& collisions);
    
    /// \param capsuleA pointer to first shape
    /// \param sphereB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleSphere(const CapsuleShape* capsuleA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape
    /// \param capsuleB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleCapsule(const CapsuleShape* capsuleA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape
    /// \param planeB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsulePlane(const CapsuleShape* capsuleA, const PlaneShape* planeB, CollisionList& collisions);
    
    /// \param planeA pointer to first shape
    /// \param sphereB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeSphere(const PlaneShape* planeA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param planeA pointer to first shape
    /// \param capsuleB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeCapsule(const PlaneShape* planeA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param planeA pointer to first shape
    /// \param planeB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planePlane(const PlaneShape* planeA, const PlaneShape* planeB, CollisionList& collisions);
    
    /// \param sphereA pointer to first shape
    /// \param listB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool sphereList(const SphereShape* sphereA, const ListShape* listB, CollisionList& collisions);

    /// \param capuleA pointer to first shape
    /// \param listB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool capsuleList(const CapsuleShape* capsuleA, const ListShape* listB, CollisionList& collisions);

    /// \param planeA pointer to first shape
    /// \param listB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool planeList(const PlaneShape* planeA, const ListShape* listB, CollisionList& collisions);
    
    /// \param listA pointer to first shape
    /// \param sphereB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listSphere(const ListShape* listA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param listA pointer to first shape
    /// \param capsuleB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listCapsule(const ListShape* listA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param listA pointer to first shape
    /// \param planeB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listPlane(const ListShape* listA, const PlaneShape* planeB, CollisionList& collisions);
    
    /// \param listA pointer to first shape
    /// \param capsuleB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listList(const ListShape* listA, const ListShape* listB, CollisionList& collisions);

}   // namespace ShapeCollider

#endif // hifi_ShapeCollider_h
