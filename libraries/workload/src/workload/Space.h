//
//  Space.h
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.01.30
//  Copyright 2018 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple plane class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_workload_Space_h
#define hifi_workload_Space_h

#include <memory>
#include <vector>
#include <glm/glm.hpp>


namespace workload {

class Space {
public:
    static const int32_t NUM_CLASSIFICATIONS = 4;
    static const int32_t NUM_TRANSITIONS = NUM_CLASSIFICATIONS * (NUM_CLASSIFICATIONS - 1);

    static int32_t computeTransitionIndex(int32_t prevIndex, int32_t newIndex);

    static const uint8_t REGION_NEAR = 0;
    static const uint8_t REGION_MIDDLE = 1;
    static const uint8_t REGION_FAR = 2;
    static const uint8_t REGION_UNKNOWN = 3;
    static const uint8_t REGION_INVALID = 4;

    using Sphere = glm::vec4; // <x,y,z> = center, w = radius
    using ProxyUpdate = std::pair<int32_t, Sphere>;

    class Proxy {
    public:
        Proxy() : sphere(0.0f) {}
        Proxy(const Sphere& s) : sphere(s) {}
        Sphere sphere;
        uint8_t region { REGION_UNKNOWN };
        uint8_t prevRegion { REGION_UNKNOWN };
        uint16_t _padding;
        uint32_t _paddings[3];
    };

    class View {
    public:
        View(const glm::vec3& pos, float nearRadius, float midRadius, float farRadius) : center(pos) {
            radiuses[REGION_NEAR] = nearRadius;
            radiuses[REGION_MIDDLE] = midRadius;
            radiuses[REGION_FAR] = farRadius;
        }
        glm::vec3 center;
        float radiuses[NUM_CLASSIFICATIONS - 1];
    };

    class Change {
    public:
        Change(int32_t i, uint32_t c, uint32_t p) : proxyId(i), region(c), prevRegion(p) {}
        int32_t proxyId { -1 };
        uint8_t region { 0 };
        uint8_t prevRegion { 0 };
    };

    Space() {}

    void clear();
    int32_t createProxy(const Sphere& sphere);
    void deleteProxies(const std::vector<int32_t>& deadIndices);
    void updateProxies(const std::vector<ProxyUpdate>& changedProxies);
    void setViews(const std::vector<View>& views);

    uint32_t getNumObjects() const { return (uint32_t)(_proxies.size() - _freeIndices.size()); }
    uint32_t getNumAllocatedProxies() const { return (uint32_t)(_proxies.size()); }

    void categorizeAndGetChanges(std::vector<Change>& changes);
    uint32_t copyProxyValues(Proxy* proxies, uint32_t numDestProxies);

private:
    void deleteProxy(int32_t proxyId);
    void updateProxy(int32_t proxyId, const Sphere& sphere);

    std::vector<Proxy> _proxies;
    std::vector<View> _views;
    std::vector<int32_t> _freeIndices;
};

inline int32_t Space::computeTransitionIndex(int32_t prevIndex, int32_t newIndex) {
    // given prevIndex and newIndex compute an index into the transition list,
    // where the lists between unchanged indices don't exist (signaled by index = -1).
    //
    // Given an NxN array
    // let p = i + N * j
    //
    // then k = -1                   when i == j
    //        = p - (1 + p/(N+1))    when i != j
    //
    //    i   0       1       2       3
    // j  +-------+-------+-------+-------+
    //    |p = 0  |    1  |    2  |    3  |
    // 0  |       |       |       |       |
    //    |k = -1 |    0  |    1  |    2  |
    //    +-------+-------+-------+-------+
    //    |     4 |    5  |    6  |    7  |
    // 1  |       |       |       |       |
    //    |     3 |    -1 |    4  |    5  |
    //    +-------+-------+-------+-------+
    //    |     8 |     9 |   10  |   11  |
    // 2  |       |       |       |       |
    //    |     6 |     7 |    -1 |    8  |
    //    +-------+-------+-------+-------+
    //    |    12 |    13 |    14 |    15 |
    // 3  |       |       |       |       |
    //    |     9 |    10 |    11 |    -1 |
    //    +-------+-------+-------+-------+
    int32_t p = prevIndex + Space::NUM_CLASSIFICATIONS * newIndex;
    if (0 == (p % (Space::NUM_CLASSIFICATIONS + 1))) {
        return -1;
    }
    return p - (1 + p / (Space::NUM_CLASSIFICATIONS + 1));
}

using SpacePointer = std::shared_ptr<Space>;
using Changes = std::vector<Space::Change>;
using SortedChanges = std::vector<std::vector<int32_t>>;

} // namespace workload

#endif // hifi_workload_Space_h
