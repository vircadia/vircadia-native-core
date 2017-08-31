//
//  RayPickManager.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPickManager_h
#define hifi_RayPickManager_h

#include "RayPick.h"

#include <memory>
#include <QtCore/QObject>
#include <QReadWriteLock>

#include "RegisteredMetaTypes.h"

#include <unordered_map>
#include <queue>

class RayPickResult;

class RayPickManager {

public:
    void update();
    const PickRay getPickRay(const QUuid uid);

    QUuid createRayPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const RayPickFilter& filter, const float maxDistance, const bool enabled);
    QUuid createRayPick(const RayPickFilter& filter, const float maxDistance, const bool enabled);
    QUuid createRayPick(const glm::vec3& position, const glm::vec3& direction, const RayPickFilter& filter, const float maxDistance, const bool enabled);
    void removeRayPick(const QUuid uid);
    void enableRayPick(const QUuid uid);
    void disableRayPick(const QUuid uid);
    const RayPickResult getPrevRayPickResult(const QUuid uid);

    void setPrecisionPicking(QUuid uid, const bool precisionPicking);
    void setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities);
    void setIncludeEntities(QUuid uid, const QScriptValue& includeEntities);
    void setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays);
    void setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays);
    void setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars);
    void setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars);

private:
    QHash<QUuid, std::shared_ptr<RayPick>> _rayPicks;
    QHash<QUuid, std::shared_ptr<QReadWriteLock>> _rayPickLocks;
    QReadWriteLock _addLock;
    std::queue<std::pair<QUuid, std::shared_ptr<RayPick>>> _rayPicksToAdd;
    QReadWriteLock _removeLock;
    std::queue<QUuid> _rayPicksToRemove;
    QReadWriteLock _containsLock;

    typedef QHash<QPair<glm::vec3, glm::vec3>, std::unordered_map<RayPickFilter::Flags, RayPickResult>> RayPickCache;

    // Returns true if this ray exists in the cache, and if it does, update res if the cached result is closer
    bool checkAndCompareCachedResults(QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache, RayPickResult& res, const RayPickFilter::Flags& mask);
    void cacheResult(const bool intersects, const RayPickResult& resTemp, const RayPickFilter::Flags& mask, RayPickResult& res, QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache);
};

#endif // hifi_RayPickManager_h