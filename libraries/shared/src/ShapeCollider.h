//
//  ShapeCollider.h
//  hifi
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__ShapeCollider__
#define __hifi__ShapeCollider__

#include "CapsuleShape.h"
#include "CollisionInfo.h"
#include "SharedUtil.h" 
#include "SphereShape.h"

namespace ShapeCollider {

    /// \param shapeA pointer to first shape
    /// \param shapeB pointer to second shape
    /// \param[out] collision where to store collision details
    /// \return true of shapes collide
    bool shapeShape(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);

    /// \param sphereA pointer to first shape (sphere)
    /// \param sphereB pointer to second shape (sphere)
    /// \param[out] collision where to store collision details
    /// \return true of shapes collide
    bool sphereSphere(const SphereShape* sphereA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param sphereA pointer to first shape (sphere)
    /// \param capsuleB pointer to second shape (capsule)
    /// \param[out] collision where to store collision details
    /// \return true of shapes collide
    bool sphereCapsule(const SphereShape* sphereA, const CapsuleShape* capsuleB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape (capsule)
    /// \param sphereB pointer to second shape (sphere)
    /// \param[out] collision where to store collision details
    /// \return true of shapes collide
    bool capsuleSphere(const CapsuleShape* capsuleA, const SphereShape* sphereB, CollisionList& collisions);

    /// \param capsuleA pointer to first shape (capsule)
    /// \param capsuleB pointer to second shape (capsule)
    /// \param[out] collision where to store collision details
    /// \return true of shapes collide
    bool capsuleCapsule(const CapsuleShape* capsuleA, const CapsuleShape* capsuleB, CollisionList& collisions);

}   // namespace ShapeCollider

#endif // __hifi__ShapeCollider__
