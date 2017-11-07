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


#include <memory>
#include <unordered_map>
#include <queue>

#include <QtCore/QObject>

#include <RegisteredMetaTypes.h>
#include <pointers/rays/RayPick.h>


class RayPickResult;

typedef struct RayCacheKey {
    RayPickFilter::Flags mask;
    QVector<QUuid> include;
    QVector<QUuid> ignore;

    bool operator==(const RayCacheKey& other) const {
        return (mask == other.mask && include == other.include && ignore == other.ignore);
    }
} RayCacheKey;

namespace std {
    template <>
    struct hash<RayCacheKey> {
        size_t operator()(const RayCacheKey& k) const {
            return ((hash<RayPickFilter::Flags>()(k.mask) ^ (qHash(k.include) << 1)) >> 1) ^ (qHash(k.ignore) << 1);
        }
    };
}

class RayPickManager : protected ReadWriteLockable {

public:
    void update();

    QUuid createRayPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const RayPickFilter& filter, const float maxDistance, const bool enabled);
    QUuid createRayPick(const RayPickFilter& filter, const float maxDistance, const bool enabled);
    QUuid createRayPick(const glm::vec3& position, const glm::vec3& direction, const RayPickFilter& filter, const float maxDistance, const bool enabled);
    void removeRayPick(const QUuid& uid);
    void enableRayPick(const QUuid& uid) const;
    void disableRayPick(const QUuid& uid) const;
    RayPickResult getPrevRayPickResult(const QUuid& uid) const;

    void setPrecisionPicking(const QUuid& uid, bool precisionPicking) const;
    void setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignore) const;
    void setIncludeItems(const QUuid& uid, const QVector<QUuid>& include) const;

private:
    RayPick::Pointer findRayPick(const QUuid& uid) const;
    QHash<QUuid, RayPick::Pointer> _rayPicks;

    typedef QHash<QPair<glm::vec3, glm::vec3>, std::unordered_map<RayCacheKey, RayPickResult>> RayPickCache;

    // Returns true if this ray exists in the cache, and if it does, update res if the cached result is closer
    bool checkAndCompareCachedResults(QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache, RayPickResult& res, const RayCacheKey& key);
    void cacheResult(const bool intersects, const RayPickResult& resTemp, const RayCacheKey& key, RayPickResult& res, QPair<glm::vec3, glm::vec3>& ray, RayPickCache& cache);
};

#endif // hifi_RayPickManager_h