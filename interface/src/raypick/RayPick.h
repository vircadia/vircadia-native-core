//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPick_h
#define hifi_RayPick_h

#include <RegisteredMetaTypes.h>
#include <Pick.h>

class EntityItemID;
class OverlayID;

class RayPickResult : public PickResult {
public:
    RayPickResult() {}
    RayPickResult(const QVariantMap& pickVariant) : PickResult(pickVariant) {}
    RayPickResult(const IntersectionType type, const QUuid& objectID, float distance, const glm::vec3& intersection, const PickRay& searchRay, const glm::vec3& surfaceNormal = glm::vec3(NAN), const QVariantMap& extraInfo = QVariantMap()) :
        PickResult(searchRay.toVariantMap()), extraInfo(extraInfo), objectID(objectID), intersection(intersection), surfaceNormal(surfaceNormal), type(type), distance(distance), intersects(type != NONE) {
    }

    RayPickResult(const RayPickResult& rayPickResult) : PickResult(rayPickResult.pickVariant) {
        type = rayPickResult.type;
        intersects = rayPickResult.intersects;
        objectID = rayPickResult.objectID;
        distance = rayPickResult.distance;
        intersection = rayPickResult.intersection;
        surfaceNormal = rayPickResult.surfaceNormal;
        extraInfo = rayPickResult.extraInfo;
    }

    QVariantMap extraInfo;
    QUuid objectID;
    glm::vec3 intersection { NAN };
    glm::vec3 surfaceNormal { NAN };
    IntersectionType type { NONE };
    float distance { FLT_MAX };
    bool intersects { false };

    virtual QVariantMap toVariantMap() const override {
        QVariantMap toReturn;
        toReturn["type"] = type;
        toReturn["intersects"] = intersects;
        toReturn["objectID"] = objectID;
        toReturn["distance"] = distance;
        toReturn["intersection"] = vec3toVariant(intersection);
        toReturn["surfaceNormal"] = vec3toVariant(surfaceNormal);
        toReturn["searchRay"] = PickResult::toVariantMap();
        toReturn["extraInfo"] = extraInfo;
        return toReturn;
    }

    bool doesIntersect() const override { return intersects; }
    bool checkOrFilterAgainstMaxDistance(float maxDistance) override { return distance < maxDistance; }

    PickResultPointer compareAndProcessNewResult(const PickResultPointer& newRes) override {
        auto newRayRes = std::static_pointer_cast<RayPickResult>(newRes);
        if (newRayRes->distance < distance) {
            return std::make_shared<RayPickResult>(*newRayRes);
        } else {
            return std::make_shared<RayPickResult>(*this);
        }
    }

};

class RayPick : public Pick<PickRay> {

public:
    RayPick(const PickFilter& filter, float maxDistance, bool enabled) : Pick(filter, maxDistance, enabled) {}

    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const override { return std::make_shared<RayPickResult>(pickVariant); }
    PickResultPointer getEntityIntersection(const PickRay& pick) override;
    PickResultPointer getOverlayIntersection(const PickRay& pick) override;
    PickResultPointer getAvatarIntersection(const PickRay& pick) override;
    PickResultPointer getHUDIntersection(const PickRay& pick) override;

    // These are helper functions for projecting and intersecting rays
    static glm::vec3 intersectRayWithEntityXYPlane(const QUuid& entityID, const glm::vec3& origin, const glm::vec3& direction);
    static glm::vec3 intersectRayWithOverlayXYPlane(const QUuid& overlayID, const glm::vec3& origin, const glm::vec3& direction);
    static glm::vec2 projectOntoEntityXYPlane(const QUuid& entityID, const glm::vec3& worldPos, bool unNormalized = true);
    static glm::vec2 projectOntoOverlayXYPlane(const QUuid& overlayID, const glm::vec3& worldPos, bool unNormalized = true);

private:
    static glm::vec3 intersectRayWithXYPlane(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& point, const glm::quat& rotation, const glm::vec3& registration);
    static glm::vec2 projectOntoXYPlane(const glm::vec3& worldPos, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& dimensions, const glm::vec3& registrationPoint, bool unNormalized);
};

#endif // hifi_RayPick_h
