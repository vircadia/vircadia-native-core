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

#include "Application.h"
#include "EntityScriptingInterface.h"
#include "ui/overlays/Overlays.h"
#include "avatar/AvatarManager.h"
#include "scripting/HMDScriptingInterface.h"
#include "DependencyManager.h"

#include "JointRayPick.h"
#include "StaticRayPick.h"
#include "MouseRayPick.h"

bool RayPickManager::checkAndCompareCachedResults(QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache, RayPickResult& res, const RayPickFilter::Flags& mask) {
    if (cache.contains(ray) && cache[ray].find(mask) != cache[ray].end()) {
        if (cache[ray][mask].distance < res.distance) {
            res = cache[ray][mask];
        }
        return true;
    }
    return false;
}

void RayPickManager::cacheResult(const bool intersects, const RayPickResult& resTemp, const RayPickFilter::Flags& mask, RayPickResult& res, QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache) {
    if (intersects) {
        cache[ray][mask] = resTemp;
        if (resTemp.distance < res.distance) {
            res = resTemp;
        }
    } else {
        cache[ray][mask] = RayPickResult(res.searchRay);
    }
}

void RayPickManager::update() {
    QReadLocker lock(&_containsLock);
    RayPickCache results;
    for (auto& uid : _rayPicks.keys()) {
        std::shared_ptr<RayPick> rayPick = _rayPicks[uid];
        QWriteLocker lock(rayPick->getLock());
        if (!rayPick->isEnabled() || rayPick->getFilter().doesPickNothing() || rayPick->getMaxDistance() < 0.0f) {
            continue;
        }

        bool valid;
        PickRay ray = rayPick->getPickRay(valid);

        if (!valid) {
            continue;
        }

        QPair<glm::vec3, glm::vec3> rayKey = QPair<glm::vec3, glm::vec3>(ray.origin, ray.direction);
        RayPickResult res = RayPickResult(ray);

        if (rayPick->getFilter().doesPickEntities()) {
            RayToEntityIntersectionResult entityRes;
            bool fromCache = true;
            bool invisible = rayPick->getFilter().doesPickInvisible();
            bool nonCollidable = rayPick->getFilter().doesPickNonCollidable();
            RayPickFilter::Flags entityMask = rayPick->getFilter().getEntityFlags();
            if (!checkAndCompareCachedResults(rayKey, results, res, entityMask)) {
                entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersectionVector(ray, !rayPick->getFilter().doesPickCourse(),
                    rayPick->getIncludeEntites(), rayPick->getIgnoreEntites(), !invisible, !nonCollidable);
                fromCache = false;
            }

            if (!fromCache) {
                cacheResult(entityRes.intersects, RayPickResult(IntersectionType::ENTITY, entityRes.entityID, entityRes.distance, entityRes.intersection, ray, entityRes.surfaceNormal),
                    entityMask, res, rayKey, results);
            }
        }

        if (rayPick->getFilter().doesPickOverlays()) {
            RayToOverlayIntersectionResult overlayRes;
            bool fromCache = true;
            bool invisible = rayPick->getFilter().doesPickInvisible();
            bool nonCollidable = rayPick->getFilter().doesPickNonCollidable();
            RayPickFilter::Flags overlayMask = rayPick->getFilter().getOverlayFlags();
            if (!checkAndCompareCachedResults(rayKey, results, res, overlayMask)) {
                overlayRes = qApp->getOverlays().findRayIntersectionVector(ray, !rayPick->getFilter().doesPickCourse(),
                    rayPick->getIncludeOverlays(), rayPick->getIgnoreOverlays(), !invisible, !nonCollidable);
                fromCache = false;
            }

            if (!fromCache) {
                cacheResult(overlayRes.intersects, RayPickResult(IntersectionType::OVERLAY, overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, ray, overlayRes.surfaceNormal),
                    overlayMask, res, rayKey, results);
            }
        }

        if (rayPick->getFilter().doesPickAvatars()) {
            RayPickFilter::Flags avatarMask = rayPick->getFilter().getAvatarFlags();
            if (!checkAndCompareCachedResults(rayKey, results, res, avatarMask)) {
                RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersectionVector(ray, rayPick->getIncludeAvatars(), rayPick->getIgnoreAvatars());
                cacheResult(avatarRes.intersects, RayPickResult(IntersectionType::AVATAR, avatarRes.avatarID, avatarRes.distance, avatarRes.intersection, ray), avatarMask, res, rayKey, results);
            }
        }

        // Can't intersect with HUD in desktop mode
        if (rayPick->getFilter().doesPickHUD() && DependencyManager::get<HMDScriptingInterface>()->isHMDMode()) {
            RayPickFilter::Flags hudMask = rayPick->getFilter().getHUDFlags();
            if (!checkAndCompareCachedResults(rayKey, results, res, hudMask)) {
                glm::vec3 hudRes = DependencyManager::get<HMDScriptingInterface>()->calculateRayUICollisionPoint(ray.origin, ray.direction);
                cacheResult(true, RayPickResult(IntersectionType::HUD, 0, glm::distance(ray.origin, hudRes), hudRes, ray), hudMask, res, rayKey, results);
            }
        }

        if (rayPick->getMaxDistance() == 0.0f || (rayPick->getMaxDistance() > 0.0f && res.distance < rayPick->getMaxDistance())) {
            rayPick->setRayPickResult(res);
        } else {
            rayPick->setRayPickResult(RayPickResult(ray));
        }
    }
}

QUuid RayPickManager::createRayPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const RayPickFilter& filter, const float maxDistance, const bool enabled) {
    QWriteLocker lock(&_containsLock);
    QUuid id = QUuid::createUuid();
    _rayPicks[id] = std::make_shared<JointRayPick>(jointName, posOffset, dirOffset, filter, maxDistance, enabled);
    return id;
}

QUuid RayPickManager::createRayPick(const RayPickFilter& filter, const float maxDistance, const bool enabled) {
    QWriteLocker lock(&_containsLock);
    QUuid id = QUuid::createUuid();
    _rayPicks[id] = std::make_shared<MouseRayPick>(filter, maxDistance, enabled);
    return id;
}

QUuid RayPickManager::createRayPick(const glm::vec3& position, const glm::vec3& direction, const RayPickFilter& filter, const float maxDistance, const bool enabled) {
    QWriteLocker lock(&_containsLock);
    QUuid id = QUuid::createUuid();
    _rayPicks[id] = std::make_shared<StaticRayPick>(position, direction, filter, maxDistance, enabled);
    return id;
}

void RayPickManager::removeRayPick(const QUuid uid) {
    QWriteLocker lock(&_containsLock);
    _rayPicks.remove(uid);
}

void RayPickManager::enableRayPick(const QUuid uid) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker rayPickLock(rayPick.value()->getLock());
        rayPick.value()->enable();
    }
}

void RayPickManager::disableRayPick(const QUuid uid) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker rayPickLock(rayPick.value()->getLock());
        rayPick.value()->disable();
    }
}

const RayPickResult RayPickManager::getPrevRayPickResult(const QUuid uid) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QReadLocker lock(rayPick.value()->getLock());
        return rayPick.value()->getPrevRayPickResult();
    }
    return RayPickResult();
}

void RayPickManager::setPrecisionPicking(QUuid uid, const bool precisionPicking) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker lock(rayPick.value()->getLock());
        rayPick.value()->setPrecisionPicking(precisionPicking);
    }
}

void RayPickManager::setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker lock(rayPick.value()->getLock());
        rayPick.value()->setIgnoreEntities(ignoreEntities);
    }
}

void RayPickManager::setIncludeEntities(QUuid uid, const QScriptValue& includeEntities) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker lock(rayPick.value()->getLock());
        rayPick.value()->setIncludeEntities(includeEntities);
    }
}

void RayPickManager::setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker lock(rayPick.value()->getLock());
        rayPick.value()->setIgnoreOverlays(ignoreOverlays);
    }
}

void RayPickManager::setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker lock(rayPick.value()->getLock());
        rayPick.value()->setIncludeOverlays(includeOverlays);
    }
}

void RayPickManager::setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker lock(rayPick.value()->getLock());
        rayPick.value()->setIgnoreAvatars(ignoreAvatars);
    }
}

void RayPickManager::setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars) {
    QReadLocker containsLock(&_containsLock);
    auto rayPick = _rayPicks.find(uid);
    if (rayPick != _rayPicks.end()) {
        QWriteLocker lock(rayPick.value()->getLock());
        rayPick.value()->setIncludeAvatars(includeAvatars);
    }
}