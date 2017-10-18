//
//  LaserPointerManager.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointerManager_h
#define hifi_LaserPointerManager_h

#include <memory>
#include <glm/glm.hpp>

#include <shared/ReadWriteLockable.h>

#include "LaserPointer.h"

class RayPickResult;


class LaserPointerManager : protected ReadWriteLockable {

public:
    QUuid createLaserPointer(const QVariant& rayProps, const LaserPointer::RenderStateMap& renderStates, const LaserPointer::DefaultRenderStateMap& defaultRenderStates,
        const bool faceAvatar, const bool centerEndY, const bool lockEnd, const bool distanceScaleEnd, const bool enabled);

    void removeLaserPointer(const QUuid& uid);
    void enableLaserPointer(const QUuid& uid) const;
    void disableLaserPointer(const QUuid& uid) const;
    void setRenderState(const QUuid& uid, const std::string& renderState) const;
    void editRenderState(const QUuid& uid, const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) const;
    const RayPickResult getPrevRayPickResult(const QUuid& uid) const;

    void setPrecisionPicking(const QUuid& uid, const bool precisionPicking) const;
    void setLaserLength(const QUuid& uid, const float laserLength) const;
    void setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignoreEntities) const;
    void setIncludeItems(const QUuid& uid, const QVector<QUuid>& includeEntities) const;

    void setLockEndUUID(const QUuid& uid, const QUuid& objectID, const bool isOverlay) const;

    void update();

private:
    LaserPointer::Pointer find(const QUuid& uid) const;
    QHash<QUuid, std::shared_ptr<LaserPointer>> _laserPointers;
};

#endif // hifi_LaserPointerManager_h
