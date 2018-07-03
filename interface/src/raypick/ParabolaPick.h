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
        PickResult(parabola.toVariantMap()), type(type), intersects(type != NONE), objectID(objectID), distance(distance), parabolicDistance(parabolicDistance), intersection(intersection), surfaceNormal(surfaceNormal), extraInfo(extraInfo) {
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

    IntersectionType type { NONE };
    bool intersects { false };
    QUuid objectID;
    float distance { FLT_MAX };
    float parabolicDistance { FLT_MAX };
    glm::vec3 intersection { NAN };
    glm::vec3 surfaceNormal { NAN };
    QVariantMap extraInfo;

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
    bool checkOrFilterAgainstMaxDistance(float maxDistance) override { return distance < maxDistance; }

    PickResultPointer compareAndProcessNewResult(const PickResultPointer& newRes) override {
        auto newParabolaRes = std::static_pointer_cast<ParabolaPickResult>(newRes);
        if (newParabolaRes->distance < distance) {
            return std::make_shared<ParabolaPickResult>(*newParabolaRes);
        } else {
            return std::make_shared<ParabolaPickResult>(*this);
        }
    }

};

class ParabolaPick : public Pick<PickParabola> {

public:
    ParabolaPick(float speed, const glm::vec3& accelerationAxis, bool rotateWithAvatar, const PickFilter& filter, float maxDistance, bool enabled) :
        Pick(filter, maxDistance, enabled), _speed(speed), _accelerationAxis(accelerationAxis), _rotateWithAvatar(rotateWithAvatar) {}

    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const override { return std::make_shared<ParabolaPickResult>(pickVariant); }
    PickResultPointer getEntityIntersection(const PickParabola& pick) override;
    PickResultPointer getOverlayIntersection(const PickParabola& pick) override;
    PickResultPointer getAvatarIntersection(const PickParabola& pick) override;
    PickResultPointer getHUDIntersection(const PickParabola& pick) override;

protected:
    float _speed;
    glm::vec3 _accelerationAxis;
    bool _rotateWithAvatar;

    glm::vec3 getAcceleration() const;
};

#endif // hifi_ParabolaPick_h
