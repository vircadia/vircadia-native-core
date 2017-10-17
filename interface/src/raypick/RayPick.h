//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPick_h
#define hifi_RayPick_h

#include "Pick.h"

class EntityItemID;
class OverlayID;

class RayPick : public Pick<PickRay> {

public:
    RayPick(const PickFilter& filter, const float maxDistance, const bool enabled) : Pick(filter, maxDistance, enabled) {}

    RayToEntityIntersectionResult getEntityIntersection(const PickRay& pick, bool precisionPicking,
                                                        const QVector<EntityItemID>& entitiesToInclude,
                                                        const QVector<EntityItemID>& entitiesToIgnore,
                                                        bool visibleOnly, bool collidableOnly) override;
    RayToOverlayIntersectionResult getOverlayIntersection(const PickRay& pick, bool precisionPicking,
                                                          const QVector<OverlayID>& overlaysToInclude,
                                                          const QVector<OverlayID>& overlaysToIgnore,
                                                          bool visibleOnly, bool collidableOnly) override;
    RayToAvatarIntersectionResult getAvatarIntersection(const PickRay& pick,
                                                        const QVector<EntityItemID>& avatarsToInclude,
                                                        const QVector<EntityItemID>& avatarsToIgnore) override;
    glm::vec3 getHUDIntersection(const PickRay& pick) override;
};

#endif // hifi_RayPick_h
