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

bool RayPickManager::checkAndCompareCachedResults(QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache, RayPickResult& res, unsigned int mask) {
    if (cache.contains(ray) && cache[ray].contains(mask)) {
        if (cache[ray][mask].distance < res.distance) {
            res = cache[ray][mask];
        }
        return true;
    }
    return false;
}

void RayPickManager::cacheResult(const bool intersects, const RayPickResult& resTemp, unsigned int mask, RayPickResult& res, QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache) {
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
    RayPickCache results;
    for (auto& uid : _rayPicks.keys()) {
        std::shared_ptr<RayPick> rayPick = _rayPicks[uid];
        if (!rayPick->isEnabled() || rayPick->getFilter() == RayPickMask::PICK_NOTHING || rayPick->getMaxDistance() < 0.0f) {
            continue;
        }

        bool valid;
        PickRay ray = rayPick->getPickRay(valid);

        if (!valid) {
            continue;
        }

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
                cacheResult(entityRes.intersects, RayPickResult(IntersectionType::ENTITY, entityRes.entityID, entityRes.distance, entityRes.intersection, entityRes.surfaceNormal),
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
                cacheResult(overlayRes.intersects, RayPickResult(IntersectionType::OVERLAY, overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, overlayRes.surfaceNormal),
                    RayPickMask::PICK_OVERLAYS | mask, res, rayKey, results);
            }
        }

        if (rayPick->getFilter() & RayPickMask::PICK_AVATARS) {
            if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_AVATARS)) {
                RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersection(ray, QScriptValue(), QScriptValue());
                cacheResult(avatarRes.intersects, RayPickResult(IntersectionType::AVATAR, avatarRes.avatarID, avatarRes.distance, avatarRes.intersection), RayPickMask::PICK_AVATARS, res, rayKey, results);
            }
        }

        // Can't intersect with HUD in desktop mode
        if (rayPick->getFilter() & RayPickMask::PICK_HUD && DependencyManager::get<HMDScriptingInterface>()->isHMDMode()) {
            if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_HUD)) {
                glm::vec3 hudRes = DependencyManager::get<HMDScriptingInterface>()->calculateRayUICollisionPoint(ray.origin, ray.direction);
                cacheResult(true, RayPickResult(IntersectionType::HUD, 0, glm::distance(ray.origin, hudRes), hudRes), RayPickMask::PICK_HUD, res, rayKey, results);
            }
        }

        QWriteLocker lock(_rayPickLocks[uid].get());
        if (rayPick->getMaxDistance() == 0.0f || (rayPick->getMaxDistance() > 0.0f && res.distance < rayPick->getMaxDistance())) {
            rayPick->setRayPickResult(res);
        } else {
            rayPick->setRayPickResult(RayPickResult());
        }
    }

    QWriteLocker containsLock(&_containsLock);
    {
        QWriteLocker lock(&_addLock);
        while (!_rayPicksToAdd.isEmpty()) {
            QPair<unsigned int, std::shared_ptr<RayPick>> rayPickToAdd = _rayPicksToAdd.dequeue();
            _rayPicks[rayPickToAdd.first] = rayPickToAdd.second;
            _rayPickLocks[rayPickToAdd.first] = std::make_shared<QReadWriteLock>();
        }
    }

    {
        QWriteLocker lock(&_removeLock);
        while (!_rayPicksToRemove.isEmpty()) {
            unsigned int uid = _rayPicksToRemove.dequeue();
            _rayPicks.remove(uid);
            _rayPickLocks.remove(uid);
        }
    }
}

unsigned int RayPickManager::addRayPick(std::shared_ptr<RayPick> rayPick) {
    QWriteLocker lock(&_addLock);
    _rayPicksToAdd.enqueue(QPair<unsigned int, std::shared_ptr<RayPick>>(_nextUID, rayPick));
    return _nextUID++;
}

void RayPickManager::removeRayPick(const unsigned int uid) {
    QWriteLocker lock(&_removeLock);
    _rayPicksToRemove.enqueue(uid);
}

void RayPickManager::enableRayPick(const unsigned int uid) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QWriteLocker rayPickLock(_rayPickLocks[uid].get());
        _rayPicks[uid]->enable();
    }
}

void RayPickManager::disableRayPick(const unsigned int uid) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QWriteLocker rayPickLock(_rayPickLocks[uid].get());
        _rayPicks[uid]->disable();
    }
}

const PickRay RayPickManager::getPickRay(const unsigned int uid) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        bool valid;
        PickRay pickRay = _rayPicks[uid]->getPickRay(valid);
        if (valid) {
            return pickRay;
        }
    }
    return PickRay();
}

const RayPickResult RayPickManager::getPrevRayPickResult(const unsigned int uid) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QReadLocker lock(_rayPickLocks[uid].get());
        return _rayPicks[uid]->getPrevRayPickResult();
    }
    return RayPickResult();
}
