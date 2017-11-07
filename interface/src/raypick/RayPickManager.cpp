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

#include <pointers/rays/StaticRayPick.h>

#include "Application.h"
#include "EntityScriptingInterface.h"
#include "ui/overlays/Overlays.h"
#include "avatar/AvatarManager.h"
#include "scripting/HMDScriptingInterface.h"
#include "DependencyManager.h"

#include "JointRayPick.h"
#include "MouseRayPick.h"

bool RayPickManager::checkAndCompareCachedResults(QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache, RayPickResult& res, const RayCacheKey& key) {
    if (cache.contains(ray) && cache[ray].find(key) != cache[ray].end()) {
        if (cache[ray][key].distance < res.distance) {
            res = cache[ray][key];
        }
        return true;
    }
    return false;
}

void RayPickManager::cacheResult(const bool intersects, const RayPickResult& resTemp, const RayCacheKey& key, RayPickResult& res, QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache) {
    if (intersects) {
        cache[ray][key] = resTemp;
        if (resTemp.distance < res.distance) {
            res = resTemp;
        }
    } else {
        cache[ray][key] = RayPickResult(res.searchRay);
    }
}

void RayPickManager::update() {
    RayPickCache results;
    QHash<QUuid, RayPick::Pointer> cachedRayPicks;
    withReadLock([&] {
        cachedRayPicks = _rayPicks;
    });

    for (const auto& uid : cachedRayPicks.keys()) {
        std::shared_ptr<RayPick> rayPick = cachedRayPicks[uid];
        if (!rayPick->isEnabled() || rayPick->getFilter().doesPickNothing() || rayPick->getMaxDistance() < 0.0f) {
            continue;
        }

        PickRay ray;

        {
            bool valid;
            ray = rayPick->getPickRay(valid);
            if (!valid) {
                continue;
            }
        }

        QPair<glm::vec3, glm::vec3> rayKey = QPair<glm::vec3, glm::vec3>(ray.origin, ray.direction);
        RayPickResult res = RayPickResult(ray);

        if (rayPick->getFilter().doesPickEntities()) {
            RayToEntityIntersectionResult entityRes;
            bool fromCache = true;
            bool invisible = rayPick->getFilter().doesPickInvisible();
            bool nonCollidable = rayPick->getFilter().doesPickNonCollidable();
            RayCacheKey entityKey = { rayPick->getFilter().getEntityFlags(), rayPick->getIncludeItems(), rayPick->getIgnoreItems() };
            if (!checkAndCompareCachedResults(rayKey, results, res, entityKey)) {
                entityRes = DependencyManager::get<EntityScriptingInterface>()->findRayIntersectionVector(ray, !rayPick->getFilter().doesPickCoarse(),
                    rayPick->getIncludeItemsAs<EntityItemID>(), rayPick->getIgnoreItemsAs<EntityItemID>(), !invisible, !nonCollidable);
                fromCache = false;
            }

            if (!fromCache) {
                cacheResult(entityRes.intersects, RayPickResult(IntersectionType::ENTITY, entityRes.entityID, entityRes.distance, entityRes.intersection, ray, entityRes.surfaceNormal),
                    entityKey, res, rayKey, results);
            }
        }

        if (rayPick->getFilter().doesPickOverlays()) {
            RayToOverlayIntersectionResult overlayRes;
            bool fromCache = true;
            bool invisible = rayPick->getFilter().doesPickInvisible();
            bool nonCollidable = rayPick->getFilter().doesPickNonCollidable();
            RayCacheKey overlayKey = { rayPick->getFilter().getOverlayFlags(), rayPick->getIncludeItems(), rayPick->getIgnoreItems() };
            if (!checkAndCompareCachedResults(rayKey, results, res, overlayKey)) {
                overlayRes = qApp->getOverlays().findRayIntersectionVector(ray, !rayPick->getFilter().doesPickCoarse(),
                    rayPick->getIncludeItemsAs<OverlayID>(), rayPick->getIgnoreItemsAs<OverlayID>(), !invisible, !nonCollidable);
                fromCache = false;
            }

            if (!fromCache) {
                cacheResult(overlayRes.intersects, RayPickResult(IntersectionType::OVERLAY, overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, ray, overlayRes.surfaceNormal),
                    overlayKey, res, rayKey, results);
            }
        }

        if (rayPick->getFilter().doesPickAvatars()) {
            RayCacheKey avatarKey = { rayPick->getFilter().getAvatarFlags(), rayPick->getIncludeItems(), rayPick->getIgnoreItems() };
            if (!checkAndCompareCachedResults(rayKey, results, res, avatarKey)) {
                RayToAvatarIntersectionResult avatarRes = DependencyManager::get<AvatarManager>()->findRayIntersectionVector(ray, 
                    rayPick->getIncludeItemsAs<EntityItemID>(), rayPick->getIgnoreItemsAs<EntityItemID>());
                cacheResult(avatarRes.intersects, RayPickResult(IntersectionType::AVATAR, avatarRes.avatarID, avatarRes.distance, avatarRes.intersection, ray), avatarKey, res, rayKey, results);
            }
        }

        // Can't intersect with HUD in desktop mode
        if (rayPick->getFilter().doesPickHUD() && DependencyManager::get<HMDScriptingInterface>()->isHMDMode()) {
            RayCacheKey hudKey = { rayPick->getFilter().getHUDFlags(), QVector<QUuid>(), QVector<QUuid>() };
            if (!checkAndCompareCachedResults(rayKey, results, res, hudKey)) {
                glm::vec3 hudRes = DependencyManager::get<HMDScriptingInterface>()->calculateRayUICollisionPoint(ray.origin, ray.direction);
                cacheResult(true, RayPickResult(IntersectionType::HUD, 0, glm::distance(ray.origin, hudRes), hudRes, ray), hudKey, res, rayKey, results);
            }
        }

        if (rayPick->getMaxDistance() == 0.0f || (rayPick->getMaxDistance() > 0.0f && res.distance < rayPick->getMaxDistance())) {
            rayPick->setRayPickResult(res);
        } else {
            rayPick->setRayPickResult(RayPickResult(ray));
        }
    }
}

QUuid RayPickManager::createRayPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const RayPickFilter& filter, float maxDistance, bool enabled) {
    auto newRayPick = std::make_shared<JointRayPick>(jointName, posOffset, dirOffset, filter, maxDistance, enabled);
    QUuid id = QUuid::createUuid();
    withWriteLock([&] {
        _rayPicks[id] = newRayPick;
    });
    return id;
}

QUuid RayPickManager::createRayPick(const RayPickFilter& filter, float maxDistance, bool enabled) {
    QUuid id = QUuid::createUuid();
    auto newRayPick = std::make_shared<MouseRayPick>(filter, maxDistance, enabled);
    withWriteLock([&] {
        _rayPicks[id] = newRayPick;
    });
    return id;
}

QUuid RayPickManager::createRayPick(const glm::vec3& position, const glm::vec3& direction, const RayPickFilter& filter, float maxDistance, bool enabled) {
    QUuid id = QUuid::createUuid();
    auto newRayPick = std::make_shared<StaticRayPick>(position, direction, filter, maxDistance, enabled);
    withWriteLock([&] {
        _rayPicks[id] = newRayPick;
    });
    return id;
}

void RayPickManager::removeRayPick(const QUuid& uid) {
    withWriteLock([&] {
        _rayPicks.remove(uid);
    });
}

RayPick::Pointer RayPickManager::findRayPick(const QUuid& uid) const {
    return resultWithReadLock<RayPick::Pointer>([&] {
        if (_rayPicks.contains(uid)) {
            return _rayPicks[uid];
        }
        return RayPick::Pointer();
    });
}

void RayPickManager::enableRayPick(const QUuid& uid) const {
    auto rayPick = findRayPick(uid);
    if (rayPick) {
        rayPick->enable();
    }
}

void RayPickManager::disableRayPick(const QUuid& uid) const {
    auto rayPick = findRayPick(uid);
    if (rayPick) {
        rayPick->disable();
    }
}

RayPickResult RayPickManager::getPrevRayPickResult(const QUuid& uid) const {
    auto rayPick = findRayPick(uid);
    if (rayPick) {
        return rayPick->getPrevRayPickResult();
    }
    return RayPickResult();
}

void RayPickManager::setPrecisionPicking(const QUuid& uid, bool precisionPicking) const {
    auto rayPick = findRayPick(uid);
    if (rayPick) {
        rayPick->setPrecisionPicking(precisionPicking);
    }
}

void RayPickManager::setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignore) const {
    auto rayPick = findRayPick(uid);
    if (rayPick) {
        rayPick->setIgnoreItems(ignore);
    }
}

void RayPickManager::setIncludeItems(const QUuid& uid, const QVector<QUuid>& include) const {
    auto rayPick = findRayPick(uid);
    if (rayPick) {
        rayPick->setIncludeItems(include);
    }
}
