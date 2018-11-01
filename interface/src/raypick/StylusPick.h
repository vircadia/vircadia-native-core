//
//  Created by Bradley Austin Davis on 2017/10/24
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_StylusPick_h
#define hifi_StylusPick_h

#include <Pick.h>
#include "RegisteredMetaTypes.h"

class StylusPickResult : public PickResult {
    using Side = bilateral::Side;

public:
    StylusPickResult() {}
    StylusPickResult(const QVariantMap& pickVariant) : PickResult(pickVariant) {}
    StylusPickResult(const IntersectionType type, const QUuid& objectID, float distance, const glm::vec3& intersection, const StylusTip& stylusTip,
        const glm::vec3& surfaceNormal = glm::vec3(NAN)) :
        PickResult(stylusTip.toVariantMap()), type(type), intersects(type != NONE), objectID(objectID), distance(distance), intersection(intersection), surfaceNormal(surfaceNormal) {
    }

    StylusPickResult(const StylusPickResult& stylusPickResult) : PickResult(stylusPickResult.pickVariant) {
        type = stylusPickResult.type;
        intersects = stylusPickResult.intersects;
        objectID = stylusPickResult.objectID;
        distance = stylusPickResult.distance;
        intersection = stylusPickResult.intersection;
        surfaceNormal = stylusPickResult.surfaceNormal;
    }

    IntersectionType type { NONE };
    bool intersects { false };
    QUuid objectID;
    float distance { FLT_MAX };
    glm::vec3 intersection { NAN };
    glm::vec3 surfaceNormal { NAN };

    virtual QVariantMap toVariantMap() const override {
        QVariantMap toReturn;
        toReturn["type"] = type;
        toReturn["intersects"] = intersects;
        toReturn["objectID"] = objectID;
        toReturn["distance"] = distance;
        toReturn["intersection"] = vec3toVariant(intersection);
        toReturn["surfaceNormal"] = vec3toVariant(surfaceNormal);
        toReturn["stylusTip"] = PickResult::toVariantMap();
        return toReturn;
    }

    bool doesIntersect() const override { return intersects; }
    std::shared_ptr<PickResult> compareAndProcessNewResult(const std::shared_ptr<PickResult>& newRes) override;
    bool checkOrFilterAgainstMaxDistance(float maxDistance) override;
};

class StylusPick : public Pick<StylusTip> {
    using Side = bilateral::Side;
public:
    StylusPick(Side side, const PickFilter& filter, float maxDistance, bool enabled, const glm::vec3& tipOffset);

    StylusTip getMathematicalPick() const override;
    PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const override;
    PickResultPointer getEntityIntersection(const StylusTip& pick) override;
    PickResultPointer getOverlayIntersection(const StylusTip& pick) override;
    PickResultPointer getAvatarIntersection(const StylusTip& pick) override;
    PickResultPointer getHUDIntersection(const StylusTip& pick) override;
    Transform getResultTransform() const override;

    bool isLeftHand() const override { return _mathPick.side == Side::Left; }
    bool isRightHand() const override { return _mathPick.side == Side::Right; }
    bool isMouse() const override { return false; }

    static float WEB_STYLUS_LENGTH;
};

#endif // hifi_StylusPick_h
