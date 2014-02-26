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
#include "ListShape.h"
#include "SharedUtil.h" 
#include "SphereShape.h"

namespace ShapeCollider {

    /// \param shapeA pointer to first shape
    /// \param shapeB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool shapeShape(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions);

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
    /// \param capsuleB pointer to second shape
    /// \param[out] collisions where to append collision details
    /// \return true if shapes collide
    bool listList(const ListShape* listA, const ListShape* listB, CollisionList& collisions);

}   // namespace ShapeCollider

#endif // __hifi__ShapeCollider__
