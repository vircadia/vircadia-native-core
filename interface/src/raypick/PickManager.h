//
//  PickManager.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 10/16/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PickManager_h
#define hifi_PickManager_h

#include <unordered_map>

#include "scripting/HMDScriptingInterface.h"

#include "Pick.h"

typedef struct PickCacheKey {
    PickFilter::Flags mask;
    QVector<QUuid> include;
    QVector<QUuid> ignore;

    bool operator==(const PickCacheKey& other) const {
        return (mask == other.mask && include == other.include && ignore == other.ignore);
    }
} PickCacheKey;

namespace std {
    template <>
    struct hash<PickCacheKey> {
        size_t operator()(const PickCacheKey& k) const {
            return ((hash<PickFilter::Flags>()(k.mask) ^ (qHash(k.include) << 1)) >> 1) ^ (qHash(k.ignore) << 1);
        }
    };
}

// T is a mathematical representation of a Pick.
// For example: PickRay for RayPick
// TODO: add another template type to replace RayPickResult with a generalized PickResult
template<typename T>
class PickManager : protected ReadWriteLockable {

public:
    virtual void update();

    RayPickResult getPrevPickResult(const QUuid& uid) const;

    QUuid addPick(const std::shared_ptr<Pick<T>> pick);
    void removePick(const QUuid& uid);
    void enablePick(const QUuid& uid) const;
    void disablePick(const QUuid& uid) const;

    void setPrecisionPicking(const QUuid& uid, bool precisionPicking) const;
    void setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignore) const;
    void setIncludeItems(const QUuid& uid, const QVector<QUuid>& include) const;

protected:
    std::shared_ptr<Pick<T>> findPick(const QUuid& uid) const;
    QHash<QUuid, std::shared_ptr<Pick<T>>> _picks;

    typedef std::unordered_map<T, std::unordered_map<PickCacheKey, RayPickResult>> PickCache;

    // Returns true if this ray exists in the cache, and if it does, update res if the cached result is closer
    bool checkAndCompareCachedResults(T& pick, PickCache& cache, RayPickResult& res, const PickCacheKey& key);
    void cacheResult(const bool intersects, const RayPickResult& resTemp, const PickCacheKey& key, RayPickResult& res, T& pick, PickCache& cache);
};

template<typename T>
std::shared_ptr<Pick<T>> PickManager<T>::findPick(const QUuid& uid) const {
    return resultWithReadLock<std::shared_ptr<Pick<T>>>([&] {
        if (_picks.contains(uid)) {
            return _picks[uid];
        }
        return std::shared_ptr<Pick<T>>();
    });
}

template<typename T>
bool PickManager<T>::checkAndCompareCachedResults(T& pick, PickCache& cache, RayPickResult& res, const PickCacheKey& key) {
    if (cache.find(pick) != cache.end() && cache[pick].find(key) != cache[pick].end()) {
        if (cache[pick][key].distance < res.distance) {
            res = cache[pick][key];
        }
        return true;
    }
    return false;
}

template<typename T>
void PickManager<T>::cacheResult(const bool intersects, const RayPickResult& resTemp, const PickCacheKey& key, RayPickResult& res, T& pick, PickCache& cache) {
    if (intersects) {
        cache[pick][key] = resTemp;
        if (resTemp.distance < res.distance) {
            res = resTemp;
        }
    } else {
        cache[pick][key] = RayPickResult(res.searchRay);
    }
}

template<typename T>
void PickManager<T>::update() {
    PickCache results;
    QHash<QUuid, std::shared_ptr<Pick<T>>> cachedPicks;
    withReadLock([&] {
        cachedPicks = _picks;
    });

    for (const auto& uid : cachedPicks.keys()) {
        std::shared_ptr<Pick<T>> pick = cachedPicks[uid];
        if (!pick->isEnabled() || pick->getFilter().doesPickNothing() || pick->getMaxDistance() < 0.0f) {
            continue;
        }

        T mathematicalPick = pick->getMathematicalPick();

        if (!mathematicalPick) {
            continue;
        }

        RayPickResult res = RayPickResult(mathematicalPick);

        if (pick->getFilter().doesPickEntities()) {
            RayToEntityIntersectionResult entityRes;
            bool fromCache = true;
            bool invisible = pick->getFilter().doesPickInvisible();
            bool nonCollidable = pick->getFilter().doesPickNonCollidable();
            PickCacheKey entityKey = { pick->getFilter().getEntityFlags(), pick->getIncludeItems(), pick->getIgnoreItems() };
            if (!checkAndCompareCachedResults(mathematicalPick, results, res, entityKey)) {
                entityRes = pick->getEntityIntersection(mathematicalPick, !pick->getFilter().doesPickCoarse(),
                    pick->getIncludeItemsAs<EntityItemID>(), pick->getIgnoreItemsAs<EntityItemID>(), !invisible, !nonCollidable);
                fromCache = false;
            }

            if (!fromCache) {
                cacheResult(entityRes.intersects, RayPickResult(IntersectionType::ENTITY, entityRes.entityID, entityRes.distance, entityRes.intersection, mathematicalPick, entityRes.surfaceNormal),
                    entityKey, res, mathematicalPick, results);
            }
        }

        if (pick->getFilter().doesPickOverlays()) {
            RayToOverlayIntersectionResult overlayRes;
            bool fromCache = true;
            bool invisible = pick->getFilter().doesPickInvisible();
            bool nonCollidable = pick->getFilter().doesPickNonCollidable();
            PickCacheKey overlayKey = { pick->getFilter().getOverlayFlags(), pick->getIncludeItems(), pick->getIgnoreItems() };
            if (!checkAndCompareCachedResults(mathematicalPick, results, res, overlayKey)) {
                overlayRes = pick->getOverlayIntersection(mathematicalPick, !pick->getFilter().doesPickCoarse(),
                    pick->getIncludeItemsAs<OverlayID>(), pick->getIgnoreItemsAs<OverlayID>(), !invisible, !nonCollidable);
                fromCache = false;
            }

            if (!fromCache) {
                cacheResult(overlayRes.intersects, RayPickResult(IntersectionType::OVERLAY, overlayRes.overlayID, overlayRes.distance, overlayRes.intersection, mathematicalPick, overlayRes.surfaceNormal),
                    overlayKey, res, mathematicalPick, results);
            }
        }

        if (pick->getFilter().doesPickAvatars()) {
            PickCacheKey avatarKey = { pick->getFilter().getAvatarFlags(), pick->getIncludeItems(), pick->getIgnoreItems() };
            if (!checkAndCompareCachedResults(mathematicalPick, results, res, avatarKey)) {
                RayToAvatarIntersectionResult avatarRes = pick->getAvatarIntersection(mathematicalPick,
                    pick->getIncludeItemsAs<EntityItemID>(), pick->getIgnoreItemsAs<EntityItemID>());
                cacheResult(avatarRes.intersects, RayPickResult(IntersectionType::AVATAR, avatarRes.avatarID, avatarRes.distance, avatarRes.intersection, mathematicalPick), avatarKey, res, mathematicalPick, results);
            }
        }

        // Can't intersect with HUD in desktop mode
        if (pick->getFilter().doesPickHUD() && DependencyManager::get<HMDScriptingInterface>()->isHMDMode()) {
            PickCacheKey hudKey = { pick->getFilter().getHUDFlags(), QVector<QUuid>(), QVector<QUuid>() };
            if (!checkAndCompareCachedResults(mathematicalPick, results, res, hudKey)) {
                glm::vec3 hudRes = pick->getHUDIntersection(mathematicalPick);
                cacheResult(true, RayPickResult(IntersectionType::HUD, 0, glm::distance(mathematicalPick.origin, hudRes), hudRes, mathematicalPick), hudKey, res, mathematicalPick, results);
            }
        }

        if (pick->getMaxDistance() == 0.0f || (pick->getMaxDistance() > 0.0f && res.distance < pick->getMaxDistance())) {
            pick->setPickResult(res);
        } else {
            pick->setPickResult(RayPickResult(mathematicalPick));
        }
    }
}

template<typename T>
RayPickResult PickManager<T>::getPrevPickResult(const QUuid& uid) const {
    auto pick = findPick(uid);
    if (pick) {
        return pick->getPrevPickResult();
    }
    return RayPickResult();
}

template<typename T>
QUuid PickManager<T>::addPick(const std::shared_ptr<Pick<T>> pick) {
    QUuid id = QUuid::createUuid();
    withWriteLock([&] {
        _picks[id] = pick;
    });
    return id;
}

template<typename T>
void PickManager<T>::removePick(const QUuid& uid) {
    withWriteLock([&] {
        _picks.remove(uid);
    });
}

template<typename T>
void PickManager<T>::enablePick(const QUuid& uid) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->enable();
    }
}

template<typename T>
void PickManager<T>::disablePick(const QUuid& uid) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->disable();
    }
}

template<typename T>
void PickManager<T>::setPrecisionPicking(const QUuid& uid, bool precisionPicking) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setPrecisionPicking(precisionPicking);
    }
}

template<typename T>
void PickManager<T>::setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignore) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setIgnoreItems(ignore);
    }
}

template<typename T>
void PickManager<T>::setIncludeItems(const QUuid& uid, const QVector<QUuid>& include) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setIncludeItems(include);
    }
}


#endif // hifi_PickManager_h