//
//  Created by Sabrina Shanman 7/11/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_CollisionPick_h
#define hifi_CollisionPick_h

#include <btBulletDynamicsCommon.h>

#include <avatar/AvatarMotionState.h>
#include <EntityMotionState.h>
#include <BulletUtil.h>
#include <RegisteredMetaTypes.h>
#include <Pick.h>

class CollisionPickResult : public PickResult {
public:
    struct EntityIntersection {
        EntityIntersection() { }

        EntityIntersection(const EntityIntersection& entityIntersection) :
            id(entityIntersection.id),
            pickCollisionPoint(entityIntersection.pickCollisionPoint),
            entityCollisionPoint(entityIntersection.entityCollisionPoint) {
        }

        EntityIntersection(QUuid id, glm::vec3 pickCollisionPoint, glm::vec3 entityCollisionPoint) :
            id(id),
            pickCollisionPoint(pickCollisionPoint),
            entityCollisionPoint(entityCollisionPoint) {
        }

        QVariantMap toVariantMap() {
            QVariantMap variantMap;
            variantMap["objectID"] = id;
            variantMap["pickCollisionPoint"] = vec3toVariant(pickCollisionPoint);
            variantMap["entityCollisionPoint"] = vec3toVariant(entityCollisionPoint);
            return variantMap;
        }

        QUuid id;
        glm::vec3 pickCollisionPoint;
        glm::vec3 entityCollisionPoint;
    };

    CollisionPickResult() {}
    CollisionPickResult(const QVariantMap& pickVariant) : PickResult(pickVariant) {}

    CollisionPickResult(const CollisionRegion& searchRegion, const std::vector<EntityIntersection>& intersectingEntities, const std::vector<EntityIntersection>& intersectingAvatars) :
        PickResult(searchRegion.toVariantMap()),
        intersects(intersectingEntities.size() || intersectingAvatars.size()),
        intersectingEntities(intersectingEntities),
        intersectingAvatars(intersectingAvatars) {
    }

    CollisionPickResult(const CollisionPickResult& collisionPickResult) : PickResult(collisionPickResult.pickVariant) {
        intersectingAvatars = collisionPickResult.intersectingAvatars;
        intersectingEntities = collisionPickResult.intersectingEntities;
        intersects = intersectingAvatars.size() || intersectingEntities.size();
    }

    bool intersects { false };
    std::vector<EntityIntersection> intersectingEntities;
    std::vector<EntityIntersection> intersectingAvatars;

    virtual QVariantMap toVariantMap() const override {
        QVariantMap variantMap;

        variantMap["intersects"] = intersects;
        
        QVariantList qIntersectingEntities;
        for (auto intersectingEntity : intersectingEntities) {
            qIntersectingEntities.append(intersectingEntity.toVariantMap());
        }
        variantMap["intersectingEntities"] = qIntersectingEntities;

        QVariantList qIntersectingAvatars;
        for (auto intersectingAvatar : intersectingAvatars) {
            qIntersectingAvatars.append(intersectingAvatar.toVariantMap());
        }
        variantMap["intersectingAvatars"] = qIntersectingAvatars;

        variantMap["collisionRegion"] = pickVariant;

        return variantMap;
    }

    bool doesIntersect() const override { return intersects; }
    bool checkOrFilterAgainstMaxDistance(float maxDistance) override { return true; }

    PickResultPointer compareAndProcessNewResult(const PickResultPointer& newRes) override {
        const std::shared_ptr<CollisionPickResult> newCollisionResult = std::static_pointer_cast<CollisionPickResult>(*const_cast<PickResultPointer*>(&newRes));
        // Have to reference the raw pointer to work around strange type conversion errors
        CollisionPickResult* newCollisionResultRaw = const_cast<CollisionPickResult*>(newCollisionResult.get());

        for (EntityIntersection& intersectingEntity : newCollisionResultRaw->intersectingEntities) {
            intersectingEntities.push_back(intersectingEntity);
        }
        for (EntityIntersection& intersectingAvatar : newCollisionResultRaw->intersectingAvatars) {
            intersectingAvatars.push_back(intersectingAvatar);
        }

        intersects = intersectingEntities.size() || intersectingAvatars.size();

        return std::make_shared<CollisionPickResult>(*this);
    }
};

class CollisionPick : public Pick<CollisionRegion> {
public:
    CollisionPick(const PickFilter& filter, float maxDistance, bool enabled, CollisionRegion collisionRegion, const btCollisionWorld* collisionWorld) :
        Pick(filter, maxDistance, enabled),
        _mathPick(collisionRegion),
        _collisionWorld(collisionWorld) {
    }

    CollisionRegion getMathematicalPick() const override;
    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const { return std::make_shared<CollisionPickResult>(pickVariant); }
    PickResultPointer getEntityIntersection(const CollisionRegion& pick) override;
    PickResultPointer getOverlayIntersection(const CollisionRegion& pick) override;
    PickResultPointer getAvatarIntersection(const CollisionRegion& pick) override;
    PickResultPointer getHUDIntersection(const CollisionRegion& pick) override;

protected:
    // Returns true if pick.shapeInfo is valid. Otherwise, attempts to get the shapeInfo ready for use.
    bool isShapeInfoReady(CollisionRegion& pick);
    void computeShapeInfo(CollisionRegion& pick, ShapeInfo& shapeInfo, QSharedPointer<GeometryResource> resource);

    CollisionRegion _mathPick;
    const btCollisionWorld* _collisionWorld;
    QSharedPointer<GeometryResource> _cachedResource;
};

// Callback for checking the motion states of all colliding rigid bodies for candidacy to be added to a list
struct RigidBodyFilterResultCallback : public btCollisionWorld::ContactResultCallback {
    RigidBodyFilterResultCallback(const ShapeInfo& shapeInfo, const Transform& transform);

    ~RigidBodyFilterResultCallback();

    RigidBodyFilterResultCallback(btCollisionObject& testCollisionObject) :
        btCollisionWorld::ContactResultCallback(), collisionObject(testCollisionObject) {
    }

    btCollisionObject collisionObject;

    virtual bool needsCollision(btBroadphaseProxy* proxy) const;

    // Check candidacy for adding to a list
    virtual void checkOrAddCollidingState(const btMotionState* otherMotionState, btVector3& point, btVector3& otherPoint) = 0;

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0, int partId0, int index0, const btCollisionObjectWrapper* colObj1, int partId1, int index1) override;
};

// Callback for getting colliding ObjectMotionStates in the world, or optionally a more specific type.
template <typename T = ObjectMotionState>
struct AllObjectMotionStatesCallback : public RigidBodyFilterResultCallback {
    AllObjectMotionStatesCallback(const ShapeInfo& shapeInfo, const Transform& transform) : RigidBodyFilterResultCallback(shapeInfo, transform) { }

    std::vector<CollisionPickResult::EntityIntersection> intersectingObjects;

    void checkOrAddCollidingState(const btMotionState* otherMotionState, btVector3& point, btVector3& otherPoint) override;
};

#endif // hifi_CollisionPick_h