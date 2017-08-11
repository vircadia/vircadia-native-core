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
#include "DependencyManager.h"

#include <unordered_map>
#include <queue>

class RayPickResult;

class RayPickManager : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(unsigned int PICK_NOTHING READ PICK_NOTHING CONSTANT)
    Q_PROPERTY(unsigned int PICK_ENTITIES READ PICK_ENTITIES CONSTANT)
    Q_PROPERTY(unsigned int PICK_OVERLAYS READ PICK_OVERLAYS CONSTANT)
    Q_PROPERTY(unsigned int PICK_AVATARS READ PICK_AVATARS CONSTANT)
    Q_PROPERTY(unsigned int PICK_HUD READ PICK_HUD CONSTANT)
    Q_PROPERTY(unsigned int PICK_BOUNDING_BOX READ PICK_BOUNDING_BOX CONSTANT)
    Q_PROPERTY(unsigned int PICK_INCLUDE_INVISIBLE READ PICK_INCLUDE_INVISIBLE CONSTANT)
    Q_PROPERTY(unsigned int PICK_INCLUDE_NONCOLLIDABLE READ PICK_INCLUDE_NONCOLLIDABLE CONSTANT)
    Q_PROPERTY(unsigned int PICK_ALL_INTERSECTIONS READ PICK_ALL_INTERSECTIONS CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_NONE READ INTERSECTED_NONE CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_ENTITY READ INTERSECTED_ENTITY CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_OVERLAY READ INTERSECTED_OVERLAY CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_AVATAR READ INTERSECTED_AVATAR CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_HUD READ INTERSECTED_HUD CONSTANT)
    SINGLETON_DEPENDENCY

public:
    void update();
    const PickRay getPickRay(const QUuid uid);

public slots:
    Q_INVOKABLE QUuid createRayPick(const QVariantMap& rayProps);
    Q_INVOKABLE void removeRayPick(const QUuid uid);
    Q_INVOKABLE void enableRayPick(const QUuid uid);
    Q_INVOKABLE void disableRayPick(const QUuid uid);
    Q_INVOKABLE const RayPickResult getPrevRayPickResult(const QUuid uid);

    Q_INVOKABLE void setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities);
    Q_INVOKABLE void setIncludeEntities(QUuid uid, const QScriptValue& includeEntities);
    Q_INVOKABLE void setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays);
    Q_INVOKABLE void setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays);
    Q_INVOKABLE void setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars);
    Q_INVOKABLE void setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars);

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

    unsigned int PICK_NOTHING() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_NOTHING); }
    unsigned int PICK_ENTITIES() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_ENTITIES); }
    unsigned int PICK_OVERLAYS() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_OVERLAYS); }
    unsigned int PICK_AVATARS() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_AVATARS); }
    unsigned int PICK_HUD() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_HUD); }
    unsigned int PICK_BOUNDING_BOX() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_COURSE); }
    unsigned int PICK_INCLUDE_INVISIBLE() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_INCLUDE_INVISIBLE); }
    unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_INCLUDE_NONCOLLIDABLE); }
    unsigned int PICK_ALL_INTERSECTIONS() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_ALL_INTERSECTIONS); }
    unsigned int INTERSECTED_NONE() { return IntersectionType::NONE; }
    unsigned int INTERSECTED_ENTITY() { return IntersectionType::ENTITY; }
    unsigned int INTERSECTED_OVERLAY() { return IntersectionType::OVERLAY; }
    unsigned int INTERSECTED_AVATAR() { return IntersectionType::AVATAR; }
    unsigned int INTERSECTED_HUD() { return IntersectionType::HUD; }

};

#endif // hifi_RayPickManager_h