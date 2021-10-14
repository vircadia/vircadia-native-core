//
//  Created by Sabrina Shanman 2018/07/11
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_CollisionPick_h
#define hifi_CollisionPick_h

#include <QtCore/QSharedPointer>

#include <PhysicsEngine.h>
#include <model-networking/ModelCache.h>
#include <RegisteredMetaTypes.h>
#include <TransformNode.h>
#include <Pick.h>

class CollisionPickResult : public PickResult {
public:
    CollisionPickResult() {}
    CollisionPickResult(const QVariantMap& pickVariant) : PickResult(pickVariant) {}

    CollisionPickResult(const CollisionRegion& searchRegion, const std::vector<ContactTestResult>& entityIntersections, const std::vector<ContactTestResult>& avatarIntersections) :
        PickResult(searchRegion.toVariantMap()),
        intersects(entityIntersections.size() || avatarIntersections.size()),
        entityIntersections(entityIntersections),
        avatarIntersections(avatarIntersections)
    {
    }

    CollisionPickResult(const CollisionPickResult& collisionPickResult) : PickResult(collisionPickResult.pickVariant) {
        avatarIntersections = collisionPickResult.avatarIntersections;
        entityIntersections = collisionPickResult.entityIntersections;
        intersects = collisionPickResult.intersects;
    }

    bool intersects { false };
    std::vector<ContactTestResult> entityIntersections;
    std::vector<ContactTestResult> avatarIntersections;

    QVariantMap toVariantMap() const override;

    bool doesIntersect() const override { return intersects; }
    bool checkOrFilterAgainstMaxDistance(float maxDistance) override { return true; }

    PickResultPointer compareAndProcessNewResult(const PickResultPointer& newRes) override;
};

class CollisionPick : public Pick<CollisionRegion> {
public:
    CollisionPick(const PickFilter& filter, float maxDistance, bool enabled, bool scaleWithParent, CollisionRegion collisionRegion, PhysicsEnginePointer physicsEngine);

    PickType getType() const override { return PickType::Collision; }
    CollisionRegion getMathematicalPick() const override;
    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const override {
        return std::make_shared<CollisionPickResult>(pickVariant, std::vector<ContactTestResult>(), std::vector<ContactTestResult>());
    }
    PickResultPointer getEntityIntersection(const CollisionRegion& pick) override;
    PickResultPointer getAvatarIntersection(const CollisionRegion& pick) override;
    PickResultPointer getHUDIntersection(const CollisionRegion& pick) override;
    Transform getResultTransform() const override;
protected:
    // Returns true if the resource for _mathPick.shapeInfo is loaded or if a resource is not needed.
    bool isLoaded() const;
    // Returns true if _mathPick.shapeInfo is valid. Otherwise, attempts to get the _mathPick ready for use.
    bool getShapeInfoReady(const CollisionRegion& pick);
    void computeShapeInfo(const CollisionRegion& pick, ShapeInfo& shapeInfo, QSharedPointer<GeometryResource> resource);
    void computeShapeInfoDimensionsOnly(const CollisionRegion& pick, ShapeInfo& shapeInfo, QSharedPointer<GeometryResource> resource);
    void filterIntersections(std::vector<ContactTestResult>& intersections) const;

    bool _scaleWithParent;

    PhysicsEnginePointer _physicsEngine;
    QSharedPointer<GeometryResource> _cachedResource;

    // Options for what information to get from collision results
    bool _includeNormals;
};

#endif // hifi_CollisionPick_h
