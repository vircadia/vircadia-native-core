//
//  Created by Sam Gondelman 10/16/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PickCacheOptimizer_h
#define hifi_PickCacheOptimizer_h

#include <unordered_map>

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

// T is a mathematical representation of a Pick (a MathPick)
// For example: RayPicks use T = PickRay
template<typename T>
class PickCacheOptimizer {

public:
    QVector4D update(std::unordered_map<uint32_t, std::shared_ptr<PickQuery>>& picks, uint32_t& nextToUpdate, uint64_t expiry, bool shouldPickHUD);

protected:
    typedef std::unordered_map<T, std::unordered_map<PickCacheKey, PickResultPointer>> PickCache;

    // Returns true if this pick exists in the cache, and if it does, update res if the cached result is closer
    bool checkAndCompareCachedResults(T& pick, PickCache& cache, PickResultPointer& res, const PickCacheKey& key);
    void cacheResult(const bool intersects, const PickResultPointer& resTemp, const PickCacheKey& key, PickResultPointer& res, T& mathPick, PickCache& cache, const std::shared_ptr<Pick<T>> pick);
};

template<typename T>
bool PickCacheOptimizer<T>::checkAndCompareCachedResults(T& pick, PickCache& cache, PickResultPointer& res, const PickCacheKey& key) {
    if (cache.find(pick) != cache.end() && cache[pick].find(key) != cache[pick].end()) {
        res = res->compareAndProcessNewResult(cache[pick][key]);
        return true;
    }
    return false;
}

template<typename T>
void PickCacheOptimizer<T>::cacheResult(const bool needToCompareResults, const PickResultPointer& resTemp, const PickCacheKey& key, PickResultPointer& res, T& mathPick, PickCache& cache, const std::shared_ptr<Pick<T>> pick) {
    if (needToCompareResults) {
        cache[mathPick][key] = resTemp;
        res = res->compareAndProcessNewResult(resTemp);
    } else {
        cache[mathPick][key] = pick->getDefaultResult(mathPick.toVariantMap());
    }
}

template<typename T>
QVector4D PickCacheOptimizer<T>::update(std::unordered_map<uint32_t, std::shared_ptr<PickQuery>>& picks,
        uint32_t& nextToUpdate, uint64_t expiry, bool shouldPickHUD) {
    QVector4D numIntersectionsComputed;
    PickCache results;
    const uint32_t INVALID_PICK_ID = 0;
    auto itr = picks.begin();
    if (nextToUpdate != INVALID_PICK_ID) {
        itr = picks.find(nextToUpdate);
        if (itr == picks.end()) {
            itr = picks.begin();
        }
    }
    uint32_t numUpdates = 0;
    while(numUpdates < picks.size()) {
        std::shared_ptr<Pick<T>> pick = std::static_pointer_cast<Pick<T>>(itr->second);
        T mathematicalPick = pick->getMathematicalPick();
        PickResultPointer res = pick->getDefaultResult(mathematicalPick.toVariantMap());

        if (!pick->isEnabled() || pick->getFilter().doesPickNothing() || pick->getMaxDistance() < 0.0f || !mathematicalPick) {
            pick->setPickResult(res);
        } else {
            if (pick->getFilter().doesPickEntities()) {
                PickCacheKey entityKey = { pick->getFilter().getEntityFlags(), pick->getIncludeItems(), pick->getIgnoreItems() };
                if (!checkAndCompareCachedResults(mathematicalPick, results, res, entityKey)) {
                    PickResultPointer entityRes = pick->getEntityIntersection(mathematicalPick);
                    numIntersectionsComputed[0]++;
                    if (entityRes) {
                        cacheResult(entityRes->doesIntersect(), entityRes, entityKey, res, mathematicalPick, results, pick);
                    }
                }
            }

            if (pick->getFilter().doesPickOverlays()) {
                PickCacheKey overlayKey = { pick->getFilter().getOverlayFlags(), pick->getIncludeItems(), pick->getIgnoreItems() };
                if (!checkAndCompareCachedResults(mathematicalPick, results, res, overlayKey)) {
                    PickResultPointer overlayRes = pick->getOverlayIntersection(mathematicalPick);
                    numIntersectionsComputed[1]++;
                    if (overlayRes) {
                        cacheResult(overlayRes->doesIntersect(), overlayRes, overlayKey, res, mathematicalPick, results, pick);
                    }
                }
            }

            if (pick->getFilter().doesPickAvatars()) {
                PickCacheKey avatarKey = { pick->getFilter().getAvatarFlags(), pick->getIncludeItems(), pick->getIgnoreItems() };
                if (!checkAndCompareCachedResults(mathematicalPick, results, res, avatarKey)) {
                    PickResultPointer avatarRes = pick->getAvatarIntersection(mathematicalPick);
                    numIntersectionsComputed[2]++;
                    if (avatarRes) {
                        cacheResult(avatarRes->doesIntersect(), avatarRes, avatarKey, res, mathematicalPick, results, pick);
                    }
                }
            }

            // Can't intersect with HUD in desktop mode
            if (pick->getFilter().doesPickHUD() && shouldPickHUD) {
                PickCacheKey hudKey = { pick->getFilter().getHUDFlags(), QVector<QUuid>(), QVector<QUuid>() };
                if (!checkAndCompareCachedResults(mathematicalPick, results, res, hudKey)) {
                    PickResultPointer hudRes = pick->getHUDIntersection(mathematicalPick);
                    numIntersectionsComputed[3]++;
                    if (hudRes) {
                        cacheResult(true, hudRes, hudKey, res, mathematicalPick, results, pick);
                    }
                }
            }

            if (pick->getMaxDistance() == 0.0f || (pick->getMaxDistance() > 0.0f && res->checkOrFilterAgainstMaxDistance(pick->getMaxDistance()))) {
                pick->setPickResult(res);
            } else {
                pick->setPickResult(pick->getDefaultResult(mathematicalPick.toVariantMap()));
            }
        }

        ++itr;
        if (itr == picks.end()) {
            itr = picks.begin();
        }
        nextToUpdate = itr->first;
        ++numUpdates;
        if (usecTimestampNow() > expiry) {
            break;
        }
    }
    return numIntersectionsComputed;
}

#endif // hifi_PickCacheOptimizer_h
