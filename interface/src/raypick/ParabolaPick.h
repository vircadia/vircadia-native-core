//
//  Created by Sam Gondelman 7/2/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_ParabolaPick_h
#define hifi_ParabolaPick_h

#include <RegisteredMetaTypes.h>
#include <Pick.h>

class EntityItemID;

class ParabolaPickResult : public PickResult {
public:
    ParabolaPickResult() {}
    ParabolaPickResult(const QVariantMap& pickVariant) : PickResult(pickVariant) {}
    ParabolaPickResult(const IntersectionType type, const QUuid& objectID, float distance, float parabolicDistance, const glm::vec3& intersection, const PickParabola& parabola,
                       const glm::vec3& surfaceNormal = glm::vec3(NAN), const QVariantMap& extraInfo = QVariantMap()) :
        PickResult(parabola.toVariantMap()), extraInfo(extraInfo), objectID(objectID), intersection(intersection), surfaceNormal(surfaceNormal), type(type), distance(distance), parabolicDistance(parabolicDistance), intersects(type != NONE) {
    }

    ParabolaPickResult(const ParabolaPickResult& parabolaPickResult) : PickResult(parabolaPickResult.pickVariant) {
        type = parabolaPickResult.type;
        intersects = parabolaPickResult.intersects;
        objectID = parabolaPickResult.objectID;
        distance = parabolaPickResult.distance;
        parabolicDistance = parabolaPickResult.parabolicDistance;
        intersection = parabolaPickResult.intersection;
        surfaceNormal = parabolaPickResult.surfaceNormal;
        extraInfo = parabolaPickResult.extraInfo;
    }

    QVariantMap extraInfo;
    QUuid objectID;
    glm::vec3 intersection { NAN };
    glm::vec3 surfaceNormal { NAN };
    IntersectionType type { NONE };
    float distance { FLT_MAX };
    float parabolicDistance { FLT_MAX };
    bool intersects { false };

    /*@jsdoc
     * An intersection result for a parabola pick.
     *
     * @typedef {object} ParabolaPickResult
     * @property {number} type - The intersection type.
     * @property {boolean} intersects - <code>true</code> if there's a valid intersection, <code>false</code> if there isn't.
     * @property {Uuid} objectID - The ID of the intersected object. <code>null</code> for HUD or invalid intersections.
     * @property {number} distance - The distance from the parabola origin to the intersection point in a straight line.
     * @property {number} parabolicDistance - The distance from the parabola origin to the intersection point along the arc of
     *     the parabola.
     * @property {Vec3} intersection - The intersection point in world coordinates.
     * @property {Vec3} surfaceNormal - The surface normal at the intersected point. All <code>NaN</code>s if <code>type ==
     *     Picks.INTERSECTED_HUD</code>.
     * @property {SubmeshIntersection} extraInfo - Additional intersection details for model objects, otherwise
     *     <code>{ }</code>.
     * @property {PickParabola} parabola - The pick parabola that was used. Valid even if there is no intersection.
     */
    virtual QVariantMap toVariantMap() const override {
        QVariantMap toReturn;
        toReturn["type"] = type;
        toReturn["intersects"] = intersects;
        toReturn["objectID"] = objectID;
        toReturn["distance"] = distance;
        toReturn["parabolicDistance"] = parabolicDistance;
        toReturn["intersection"] = vec3toVariant(intersection);
        toReturn["surfaceNormal"] = vec3toVariant(surfaceNormal);
        toReturn["parabola"] = PickResult::toVariantMap();
        toReturn["extraInfo"] = extraInfo;
        return toReturn;
    }

    bool doesIntersect() const override { return intersects; }
    bool checkOrFilterAgainstMaxDistance(float maxDistance) override { return parabolicDistance < maxDistance; }

    PickResultPointer compareAndProcessNewResult(const PickResultPointer& newRes) override {
        auto newParabolaRes = std::static_pointer_cast<ParabolaPickResult>(newRes);
        if (newParabolaRes->parabolicDistance < parabolicDistance) {
            return std::make_shared<ParabolaPickResult>(*newParabolaRes);
        } else {
            return std::make_shared<ParabolaPickResult>(*this);
        }
    }

};

class ParabolaPick : public Pick<PickParabola> {

public:
    ParabolaPick(const glm::vec3& position, const glm::vec3& direction, float speed, const glm::vec3& acceleration, bool rotateAccelerationWithAvatar, bool rotateAccelerationWithParent, bool scaleWithParent, const PickFilter& filter, float maxDistance, bool enabled);

    PickType getType() const override { return PickType::Parabola; }
    
    PickParabola getMathematicalPick() const override;

    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const override { return std::make_shared<ParabolaPickResult>(pickVariant); }
    PickResultPointer getEntityIntersection(const PickParabola& pick) override;
    PickResultPointer getAvatarIntersection(const PickParabola& pick) override;
    PickResultPointer getHUDIntersection(const PickParabola& pick) override;
    Transform getResultTransform() const override;

protected:
    bool _rotateAccelerationWithAvatar;
    bool _rotateAccelerationWithParent;
    bool _scaleWithParent;
    // Cached magnitude of _mathPick.velocity
    float _speed;
};

#endif // hifi_ParabolaPick_h
