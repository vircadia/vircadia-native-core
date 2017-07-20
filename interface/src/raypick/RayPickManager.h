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

#include <QHash>
#include <QQueue>
#include <QReadWriteLock>
#include <memory>
#include <QtCore/QObject>

#include "RegisteredMetaTypes.h"
#include "DependencyManager.h"

class RayPick;
class RayPickResult;

enum RayPickMask {
    PICK_NOTHING = 0,
    PICK_ENTITIES = 1 << 0,
    PICK_OVERLAYS = 1 << 1,
    PICK_AVATARS = 1 << 2,
    PICK_HUD = 1 << 3,

    PICK_BOUNDING_BOX = 1 << 4, // if not set, picks again physics mesh (can't pick against graphics mesh, yet)

    PICK_INCLUDE_INVISIBLE = 1 << 5, // if not set, will not intersect invisible elements, otherwise, intersects both visible and invisible elements
    PICK_INCLUDE_NONCOLLIDABLE = 1 << 6, // if not set, will not intersect noncollidable elements, otherwise, intersects both collidable and noncollidable elements

    PICK_ALL_INTERSECTIONS = 1 << 7 // if not set, returns closest intersection, otherwise, returns list of all intersections
};

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
    const PickRay getPickRay(const unsigned int uid);

public slots:
    Q_INVOKABLE unsigned int createRayPick(const QVariantMap& rayProps);
    Q_INVOKABLE void removeRayPick(const unsigned int uid);
    Q_INVOKABLE void enableRayPick(const unsigned int uid);
    Q_INVOKABLE void disableRayPick(const unsigned int uid);
    Q_INVOKABLE const RayPickResult getPrevRayPickResult(const unsigned int uid);

    Q_INVOKABLE void setIgnoreEntities(unsigned int uid, const QScriptValue& ignoreEntities);
    Q_INVOKABLE void setIncludeEntities(unsigned int uid, const QScriptValue& includeEntities);
    Q_INVOKABLE void setIgnoreOverlays(unsigned int uid, const QScriptValue& ignoreOverlays);
    Q_INVOKABLE void setIncludeOverlays(unsigned int uid, const QScriptValue& includeOverlays);
    Q_INVOKABLE void setIgnoreAvatars(unsigned int uid, const QScriptValue& ignoreAvatars);
    Q_INVOKABLE void setIncludeAvatars(unsigned int uid, const QScriptValue& includeAvatars);

private:
    QHash<unsigned int, std::shared_ptr<RayPick>> _rayPicks;
    QHash<unsigned int, std::shared_ptr<QReadWriteLock>> _rayPickLocks;
    unsigned int _nextUID { 1 }; // 0 is invalid
    QReadWriteLock _addLock;
    QQueue<QPair<unsigned int, std::shared_ptr<RayPick>>> _rayPicksToAdd;
    QReadWriteLock _removeLock;
    QQueue<unsigned int> _rayPicksToRemove;
    QReadWriteLock _containsLock;

    typedef QHash<QPair<glm::vec3, glm::vec3>, QHash<unsigned int, RayPickResult>> RayPickCache;

    // Returns true if this ray exists in the cache, and if it does, update res if the cached result is closer
    bool checkAndCompareCachedResults(QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache, RayPickResult& res, unsigned int mask);
    void cacheResult(const bool intersects, const RayPickResult& resTemp, unsigned int mask, RayPickResult& res, QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache);

    unsigned int PICK_NOTHING() { return RayPickMask::PICK_NOTHING; }
    unsigned int PICK_ENTITIES() { return RayPickMask::PICK_ENTITIES; }
    unsigned int PICK_OVERLAYS() { return RayPickMask::PICK_OVERLAYS; }
    unsigned int PICK_AVATARS() { return RayPickMask::PICK_AVATARS; }
    unsigned int PICK_HUD() { return RayPickMask::PICK_HUD; }
    unsigned int PICK_BOUNDING_BOX() { return RayPickMask::PICK_BOUNDING_BOX; }
    unsigned int PICK_INCLUDE_INVISIBLE() { return RayPickMask::PICK_INCLUDE_INVISIBLE; }
    unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return RayPickMask::PICK_INCLUDE_NONCOLLIDABLE; }
    unsigned int PICK_ALL_INTERSECTIONS() { return RayPickMask::PICK_ALL_INTERSECTIONS; }
    unsigned int INTERSECTED_NONE() { return IntersectionType::NONE; }
    unsigned int INTERSECTED_ENTITY() { return IntersectionType::ENTITY; }
    unsigned int INTERSECTED_OVERLAY() { return IntersectionType::OVERLAY; }
    unsigned int INTERSECTED_AVATAR() { return IntersectionType::AVATAR; }
    unsigned int INTERSECTED_HUD() { return IntersectionType::HUD; }

};

#endif // hifi_RayPickManager_h