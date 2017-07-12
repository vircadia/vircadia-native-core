//
//  RayPickManager.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RayPickManager.h"
#include "RayPick.h"

#include "Application.h"
#include "EntityScriptingInterface.h"
#include "ui/overlays/Overlays.h"
#include "avatar/AvatarManager.h"
#include "DependencyManager.h"

RayPickManager& RayPickManager::getInstance() {
    static RayPickManager instance;
    return instance;
}

// Returns true if this ray exists in the cache, and if it does, update res if the cached result is closer
bool RayPickManager::checkAndCompareCachedResults(std::shared_ptr<RayPick> rayPick, QPair<glm::vec3, glm::vec3>& ray, QHash<QPair<glm::vec3, glm::vec3>, QHash<unsigned int, RayPickResult>> cache, RayPickResult& res, unsigned int mask) {
    if (cache.contains(ray) && cache[ray].contains(mask)) {
        if (cache[ray][mask].getDistance() < res.getDistance() && cache[ray][mask].getDistance() < rayPick->getMaxDistance()) {
            res = cache[ray][mask];
        }
        return true;
    }
    return false;
}

void RayPickManager::update() {
    QHash<QPair<glm::vec3, glm::vec3>, QHash<unsigned int, RayPickResult>> results;
    for (auto &rayPick : _rayPicks) {
        if (!rayPick->isEnabled() || rayPick->getFilter() == RayPickMask::PICK_NOTHING || rayPick->getMaxDistance() < 0.0f) {
            continue;
        }

        PickRay ray = rayPick->getPickRay();
        // TODO:
        // get rid of this and make PickRay hashable
        QPair<glm::vec3, glm::vec3> rayKey = QPair<glm::vec3, glm::vec3>(ray.origin, ray.direction);
        RayPickResult res;  // start with FLT_MAX distance

        if (rayPick->getFilter() & RayPickMask::PICK_ENTITIES) {
            RayToEntityIntersectionResult entityRes;
            if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE && rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE) {
                if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_ENTITIES | RayPickMask::PICK_INCLUDE_INVISIBLE | RayPickMask::PICK_INCLUDE_NONCOLLIDABLE)) {
                    entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, QScriptValue(), QScriptValue(), false, false);
                }
            }
            else if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE) {
                if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_ENTITIES | RayPickMask::PICK_INCLUDE_INVISIBLE)) {
                    entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, QScriptValue(), QScriptValue(), false, true);
                }
            }
            else if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE) {
                if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_ENTITIES | RayPickMask::PICK_INCLUDE_NONCOLLIDABLE)) {
                    entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, QScriptValue(), QScriptValue(), true, false);
                }
            }
            else {
                if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_ENTITIES)) {
                    entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, QScriptValue(), QScriptValue(), true, true);
                }
            }
            if (entityRes.intersects) {
                res = RayPickResult(entityRes.entityID, entityRes.distance, entityRes.intersection, entityRes.surfaceNormal);
                // add to cache
            }
        }

        if (rayPick->getFilter() & RayPickMask::PICK_OVERLAYS) {
            RayToOverlayIntersectionResult overlayRes;
            if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE && rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE) {
                if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_OVERLAYS | RayPickMask::PICK_INCLUDE_INVISIBLE | RayPickMask::PICK_INCLUDE_NONCOLLIDABLE)) {
                    overlayRes = qApp->getOverlays().findRayIntersection(ray, true, QScriptValue(), QScriptValue(), false, false);
                }
            }
            else if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE) {
                if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_OVERLAYS | RayPickMask::PICK_INCLUDE_INVISIBLE)) {
                    overlayRes = qApp->getOverlays().findRayIntersection(ray, true, QScriptValue(), QScriptValue(), false, true);
                }
            }
            else if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE) {
                if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_OVERLAYS | RayPickMask::PICK_INCLUDE_NONCOLLIDABLE)) {
                    overlayRes = qApp->getOverlays().findRayIntersection(ray, true, QScriptValue(), QScriptValue(), true, false);
                }
            }
            else {
                if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_OVERLAYS)) {
                    overlayRes = qApp->getOverlays().findRayIntersection(ray, true, QScriptValue(), QScriptValue(), true, true);
                }
            }
            if (overlayRes.intersects) {
                res = RayPickResult(overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, overlayRes.surfaceNormal);
                // add to cache
            }
        }

        if (rayPick->getFilter() & RayPickMask::PICK_AVATARS) {
            if (!checkAndCompareCachedResults(rayPick, rayKey, results, res, RayPickMask::PICK_AVATARS)) {
                RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersection(ray, QScriptValue(), QScriptValue());
                if (avatarRes.intersects) {
                    res = RayPickResult(avatarRes.avatarID, avatarRes.distance, avatarRes.intersection);
                    // add to cache
                }
            }
        }

    }
}

unsigned int RayPickManager::addRayPick(std::shared_ptr<RayPick> rayPick) {
    // TODO:
    // use lock and defer adding to prevent issues
    _rayPicks[_nextUID] = rayPick;
    return _nextUID++;
}

void RayPickManager::removeRayPick(const unsigned int uid) {
    // TODO:
    // use lock and defer removing to prevent issues
    _rayPicks.remove(uid);
}

void RayPickManager::enableRayPick(const unsigned int uid) {
    // TODO:
    // use lock and defer enabling to prevent issues
    if (_rayPicks.contains(uid)) {
        _rayPicks[uid]->enable();
    }
}

void RayPickManager::disableRayPick(const unsigned int uid) {
    // TODO:
    // use lock and defer disabling to prevent issues
    if (_rayPicks.contains(uid)) {
        _rayPicks[uid]->disable();
    }
}
