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
#include "scripting/HMDScriptingInterface.h"
#include "DependencyManager.h"

RayPickManager& RayPickManager::getInstance() {
    static RayPickManager instance;
    return instance;
}

// Returns true if this ray exists in the cache, and if it does, update res if the cached result is closer
bool RayPickManager::checkAndCompareCachedResults(QPair<glm::vec3, glm::vec3>& ray, QHash<QPair<glm::vec3, glm::vec3>, QHash<unsigned int, RayPickResult>>& cache, RayPickResult& res, unsigned int mask) {
    if (cache.contains(ray) && cache[ray].contains(mask)) {
        if (cache[ray][mask].distance < res.distance) {
            res = cache[ray][mask];
        }
        return true;
    }
    return false;
}

void RayPickManager::cacheResult(const bool intersects, const RayPickResult& resTemp, unsigned int mask, RayPickResult& res,
    QPair<glm::vec3, glm::vec3>& ray, QHash<QPair<glm::vec3, glm::vec3>, QHash<unsigned int, RayPickResult>>& cache) {
    if (intersects) {
        cache[ray][mask] = resTemp;
        if (resTemp.distance < res.distance) {
            res = resTemp;
        }
    } else {
        cache[ray][mask] = RayPickResult();
    }
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
        RayPickResult res;

        if (rayPick->getFilter() & RayPickMask::PICK_ENTITIES) {
            RayToEntityIntersectionResult entityRes;
            bool fromCache = true;
            if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE && rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE) {
                if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_ENTITIES | RayPickMask::PICK_INCLUDE_INVISIBLE | RayPickMask::PICK_INCLUDE_NONCOLLIDABLE)) {
                    entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, QScriptValue(), QScriptValue(), false, false);
                    fromCache = false;
                }
            } else if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE) {
                if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_ENTITIES | RayPickMask::PICK_INCLUDE_INVISIBLE)) {
                    entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, QScriptValue(), QScriptValue(), false, true);
                    fromCache = false;
                }
            } else if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE) {
                if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_ENTITIES | RayPickMask::PICK_INCLUDE_NONCOLLIDABLE)) {
                    entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, QScriptValue(), QScriptValue(), true, false);
                    fromCache = false;
                }
            } else {
                if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_ENTITIES)) {
                    entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, QScriptValue(), QScriptValue(), true, true);
                    fromCache = false;
                }
            }

            if (!fromCache) {
                unsigned int mask = (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE) | (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE);
                cacheResult(entityRes.intersects, RayPickResult(entityRes.entityID, entityRes.distance, entityRes.intersection, entityRes.surfaceNormal),
                    RayPickMask::PICK_ENTITIES | mask, res, rayKey, results);
            }
        }

        if (rayPick->getFilter() & RayPickMask::PICK_OVERLAYS) {
            RayToOverlayIntersectionResult overlayRes;
            bool fromCache = true;
            if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE && rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE) {
                if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_OVERLAYS | RayPickMask::PICK_INCLUDE_INVISIBLE | RayPickMask::PICK_INCLUDE_NONCOLLIDABLE)) {
                    overlayRes = qApp->getOverlays().findRayIntersection(ray, true, QScriptValue(), QScriptValue(), false, false);
                    fromCache = false;
                }
            } else if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE) {
                if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_OVERLAYS | RayPickMask::PICK_INCLUDE_INVISIBLE)) {
                    overlayRes = qApp->getOverlays().findRayIntersection(ray, true, QScriptValue(), QScriptValue(), false, true);
                    fromCache = false;
                }
            } else if (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE) {
                if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_OVERLAYS | RayPickMask::PICK_INCLUDE_NONCOLLIDABLE)) {
                    overlayRes = qApp->getOverlays().findRayIntersection(ray, true, QScriptValue(), QScriptValue(), true, false);
                    fromCache = false;
                }
            } else {
                if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_OVERLAYS)) {
                    overlayRes = qApp->getOverlays().findRayIntersection(ray, true, QScriptValue(), QScriptValue(), true, true);
                    fromCache = false;
                }
            }

            if (!fromCache) {
                unsigned int mask = (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE) | (rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE);
                cacheResult(overlayRes.intersects, RayPickResult(overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, overlayRes.surfaceNormal),
                    RayPickMask::PICK_OVERLAYS | mask, res, rayKey, results);
            }
        }

        if (rayPick->getFilter() & RayPickMask::PICK_AVATARS) {
            if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_AVATARS)) {
                RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersection(ray, QScriptValue(), QScriptValue());
                cacheResult(avatarRes.intersects, RayPickResult(avatarRes.avatarID, avatarRes.distance, avatarRes.intersection), RayPickMask::PICK_AVATARS, res, rayKey, results);
            }
        }

        // Can't intersect with HUD in desktop mode
        if (rayPick->getFilter() & RayPickMask::PICK_HUD && DependencyManager::get<HMDScriptingInterface>()->isHMDMode()) {
            if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_HUD)) {
                glm::vec3 hudRes = DependencyManager::get<HMDScriptingInterface>()->calculateRayUICollisionPoint(ray.origin, ray.direction);
                cacheResult(true, RayPickResult(0, glm::distance(ray.origin, hudRes), hudRes), RayPickMask::PICK_HUD, res, rayKey, results);
            }
        }

        if (res.distance < rayPick->getMaxDistance()) {
            rayPick->setRayPickResult(res);
        } else {
            rayPick->setRayPickResult(RayPickResult());
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
    if (_rayPicks.contains(uid)) {
        _rayPicks[uid]->enable();
    }
}

void RayPickManager::disableRayPick(const unsigned int uid) {
    if (_rayPicks.contains(uid)) {
        _rayPicks[uid]->disable();
    }
}

const RayPickResult& RayPickManager::getPrevRayPickResult(const unsigned int uid) {
    // TODO:
    // does this need to lock the individual ray?  what happens with concurrent set/get?
    if (_rayPicks.contains(uid)) {
        return _rayPicks[uid]->getPrevRayPickResult();
    }
    return RayPickResult();
}
