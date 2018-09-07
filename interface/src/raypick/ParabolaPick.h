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
class OverlayID;

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
    ParabolaPick(float speed, const glm::vec3& accelerationAxis, bool rotateAccelerationWithAvatar, bool scaleWithAvatar, const PickFilter& filter, float maxDistance, bool enabled) :
        Pick(filter, maxDistance, enabled), _speed(speed), _accelerationAxis(accelerationAxis), _rotateAccelerationWithAvatar(rotateAccelerationWithAvatar),
        _scaleWithAvatar(scaleWithAvatar) {}

    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const override { return std::make_shared<ParabolaPickResult>(pickVariant); }
    PickResultPointer getEntityIntersection(const PickParabola& pick) override;
    PickResultPointer getOverlayIntersection(const PickParabola& pick) override;
    PickResultPointer getAvatarIntersection(const PickParabola& pick) override;
    PickResultPointer getHUDIntersection(const PickParabola& pick) override;
    Transform getResultTransform() const override;

protected:
    float _speed;
    glm::vec3 _accelerationAxis;
    bool _rotateAccelerationWithAvatar;
    bool _scaleWithAvatar;

    float getSpeed() const;
    glm::vec3 getAcceleration() const;
};

#endif // hifi_ParabolaPick_h
