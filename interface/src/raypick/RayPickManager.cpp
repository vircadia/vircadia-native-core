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

#include "JointRayPick.h"
#include "StaticRayPick.h"
#include "MouseRayPick.h"

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
            unsigned int invisible = rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE;
            unsigned int noncollidable = rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE;
            unsigned int entityMask = RayPickMask::PICK_ENTITIES | invisible | noncollidable;
            if (!checkAndCompareCachedResults(rayKey, results, res, entityMask)) {
                entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersection(ray, true, rayPick->getIncludeEntites(), rayPick->getIgnoreEntites(), !invisible, !noncollidable);
                fromCache = false;
            }

            if (!fromCache) {
                cacheResult(entityRes.intersects, RayPickResult(IntersectionType::ENTITY, entityRes.entityID, entityRes.distance, entityRes.intersection, entityRes.surfaceNormal),
                    entityMask, res, rayKey, results);
            }
        }

        if (rayPick->getFilter() & RayPickMask::PICK_OVERLAYS) {
            RayToOverlayIntersectionResult overlayRes;
            bool fromCache = true;
            unsigned int invisible = rayPick->getFilter() & RayPickMask::PICK_INCLUDE_INVISIBLE;
            unsigned int noncollidable = rayPick->getFilter() & RayPickMask::PICK_INCLUDE_NONCOLLIDABLE;
            unsigned int overlayMask = RayPickMask::PICK_OVERLAYS | invisible | noncollidable;
            if (!checkAndCompareCachedResults(rayKey, results, res, overlayMask)) {
                overlayRes = qApp->getOverlays().findRayIntersection(ray, true, rayPick->getIncludeOverlays(), rayPick->getIgnoreOverlays(), !invisible, !noncollidable);
                fromCache = false;
            }

            if (!fromCache) {
                cacheResult(overlayRes.intersects, RayPickResult(IntersectionType::OVERLAY, overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, overlayRes.surfaceNormal),
                    overlayMask, res, rayKey, results);
            }
        }

        if (rayPick->getFilter() & RayPickMask::PICK_AVATARS) {
            if (!checkAndCompareCachedResults(rayKey, results, res, RayPickMask::PICK_AVATARS)) {
                RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersection(ray, rayPick->getIncludeAvatars(), rayPick->getIgnoreAvatars());
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

unsigned int RayPickManager::createRayPick(const QVariantMap& rayProps) {
    bool enabled = false;
    if (rayProps["enabled"].isValid()) {
        enabled = rayProps["enabled"].toBool();
    }

    uint16_t filter = 0;
    if (rayProps["filter"].isValid()) {
        filter = rayProps["filter"].toUInt();
    }

    float maxDistance = 0.0f;
    if (rayProps["maxDistance"].isValid()) {
        maxDistance = rayProps["maxDistance"].toFloat();
    }

    if (rayProps["joint"].isValid()) {
        QString jointName = rayProps["joint"].toString();

        if (jointName != "Mouse") {
            // x = upward, y = forward, z = lateral
            glm::vec3 posOffset = Vectors::ZERO;
            if (rayProps["posOffset"].isValid()) {
                posOffset = vec3FromVariant(rayProps["posOffset"]);
            }

            glm::vec3 dirOffset = Vectors::UP;
            if (rayProps["dirOffset"].isValid()) {
                dirOffset = vec3FromVariant(rayProps["dirOffset"]);
            }

            QWriteLocker lock(&_addLock);
            _rayPicksToAdd.enqueue(QPair<unsigned int, std::shared_ptr<RayPick>>(_nextUID, std::make_shared<JointRayPick>(jointName, posOffset, dirOffset, filter, maxDistance, enabled)));
            return _nextUID++;
        } else {
            QWriteLocker lock(&_addLock);
            _rayPicksToAdd.enqueue(QPair<unsigned int, std::shared_ptr<RayPick>>(_nextUID, std::make_shared<MouseRayPick>(filter, maxDistance, enabled)));
            return _nextUID++;
        }
    } else if (rayProps["position"].isValid()) {
        glm::vec3 position = vec3FromVariant(rayProps["position"]);

        glm::vec3 direction = -Vectors::UP;
        if (rayProps["direction"].isValid()) {
            direction = vec3FromVariant(rayProps["direction"]);
        }

        QWriteLocker lock(&_addLock);
        _rayPicksToAdd.enqueue(QPair<unsigned int, std::shared_ptr<RayPick>>(_nextUID, std::make_shared<StaticRayPick>(position, direction, filter, maxDistance, enabled)));
        return _nextUID++;
    }

    return 0;
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

void RayPickManager::setIgnoreEntities(unsigned int uid, const QScriptValue& ignoreEntities) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QWriteLocker lock(_rayPickLocks[uid].get());
        _rayPicks[uid]->setIgnoreEntities(ignoreEntities);
    }
}

void RayPickManager::setIncludeEntities(unsigned int uid, const QScriptValue& includeEntities) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QWriteLocker lock(_rayPickLocks[uid].get());
        _rayPicks[uid]->setIncludeEntities(includeEntities);
    }
}

void RayPickManager::setIgnoreOverlays(unsigned int uid, const QScriptValue& ignoreOverlays) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QWriteLocker lock(_rayPickLocks[uid].get());
        _rayPicks[uid]->setIgnoreOverlays(ignoreOverlays);
    }
}

void RayPickManager::setIncludeOverlays(unsigned int uid, const QScriptValue& includeOverlays) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QWriteLocker lock(_rayPickLocks[uid].get());
        _rayPicks[uid]->setIncludeOverlays(includeOverlays);
    }
}

void RayPickManager::setIgnoreAvatars(unsigned int uid, const QScriptValue& ignoreAvatars) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QWriteLocker lock(_rayPickLocks[uid].get());
        _rayPicks[uid]->setIgnoreAvatars(ignoreAvatars);
    }
}

void RayPickManager::setIncludeAvatars(unsigned int uid, const QScriptValue& includeAvatars) {
    QReadLocker containsLock(&_containsLock);
    if (_rayPicks.contains(uid)) {
        QWriteLocker lock(_rayPickLocks[uid].get());
        _rayPicks[uid]->setIncludeAvatars(includeAvatars);
    }
}