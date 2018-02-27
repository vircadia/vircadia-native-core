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

#include <vector>
#include <glm/glm.hpp>

namespace workload {

class Space {
public:
    static const uint8_t REGION_NEAR = 0;
    static const uint8_t REGION_MIDDLE = 1;
    static const uint8_t REGION_FAR = 2;
    static const uint8_t REGION_UNKNOWN = 3;
    static const uint8_t REGION_INVALID = 4;

    using Sphere = glm::vec4; // <x,y,z> = center, w = radius

    class Proxy {
    public:
        Proxy(const Sphere& s) : sphere(s) {}
        Sphere sphere;
        uint8_t region { REGION_UNKNOWN };
        uint8_t prevRegion { REGION_UNKNOWN };
    };

    class View {
    public:
        View(const glm::vec3& pos, float nearRadius, float midRadius, float farRadius) : center(pos) {
            radiuses[0] = nearRadius;
            radiuses[1] = midRadius;
            radiuses[2] = farRadius;
        }
        glm::vec3 center { 0.0f, 0.0f, 0.0f };
        float radiuses[3] { 0.0f, 0.0f, 0.0f };
    };

    class Change {
    public:
        Change(int32_t i, uint32_t c, uint32_t p) : proxyId(i), region(c), prevRegion(p) {}
        int32_t proxyId { -1 };
        uint8_t region { 0 };
        uint8_t prevRegion { 0 };
    };

    Space() {}

    int32_t createProxy(const Sphere& sphere);
    void deleteProxy(int32_t proxyId);
    void updateProxy(int32_t proxyId, const Sphere& sphere);
    void setViews(const std::vector<View>& views);

    uint32_t getNumObjects() const { return (uint32_t)(_proxies.size() - _freeIndices.size()); }

    void categorizeAndGetChanges(std::vector<Change>& changes);

private:
    std::vector<Proxy> _proxies;
    std::vector<View> _views;
    std::vector<int32_t> _freeIndices;
};

} // namespace workload

#endif // hifi_workload_Space_h
