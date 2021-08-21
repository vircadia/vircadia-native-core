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

    /*@jsdoc
     * An intersection result for a ray pick.
     *
     * @typedef {object} RayPickResult
     * @property {IntersectionType} type - The intersection type.
     * @property {boolean} intersects - <code>true</code> if there's a valid intersection, <code>false</code> if there isn't.
     * @property {Uuid} objectID - The ID of the intersected object. <code>null</code> for HUD or invalid intersections.
     * @property {number} distance - The distance from the ray origin to the intersection point.
     * @property {Vec3} intersection - The intersection point in world coordinates.
     * @property {Vec3} surfaceNormal - The surface normal at the intersected point. All <code>NaN</code>s if <code>type == 
     *     Picks.INTERSECTED_HUD</code>.
     * @property {SubmeshIntersection} extraInfo - Additional intersection details for model objects, otherwise
     *     <code>{ }</code>.
     * @property {PickRay} searchRay - The pick ray that was used. Valid even if there is no intersection.
     */
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
    RayPick(glm::vec3 position, glm::vec3 direction, const PickFilter& filter, float maxDistance, bool enabled) :
        Pick(PickRay(position, direction), filter, maxDistance, enabled) {
    }

    PickType getType() const override { return PickType::Ray; }

    PickRay getMathematicalPick() const override;

    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const override { return std::make_shared<RayPickResult>(pickVariant); }
    PickResultPointer getEntityIntersection(const PickRay& pick) override;
    PickResultPointer getAvatarIntersection(const PickRay& pick) override;
    PickResultPointer getHUDIntersection(const PickRay& pick) override;
    Transform getResultTransform() const override;

    // These are helper functions for projecting and intersecting rays
    static glm::vec3 intersectRayWithEntityXYPlane(const QUuid& entityID, const glm::vec3& origin, const glm::vec3& direction);
    static glm::vec2 projectOntoEntityXYPlane(const QUuid& entityID, const glm::vec3& worldPos, bool unNormalized = true);

    static glm::vec2 projectOntoXYPlane(const glm::vec3& worldPos, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& dimensions, const glm::vec3& registrationPoint, bool unNormalized);
    static glm::vec2 projectOntoXZPlane(const glm::vec3& worldPos, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& dimensions, const glm::vec3& registrationPoint, bool unNoemalized);

private:
    static glm::vec3 intersectRayWithXYPlane(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& point, const glm::quat& rotation, const glm::vec3& registration);
};

#endif // hifi_RayPick_h
