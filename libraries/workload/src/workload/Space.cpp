//
//  Space.h
//  libraries/shared/src/
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

#include "Space.h"

#include <algorithm>

#include <glm/gtx/quaternion.hpp>

using namespace workload;

int32_t Space::createProxy(const Space::Sphere& newSphere) {
    if (_freeIndices.empty()) {
        _proxies.emplace_back(Space::Proxy(newSphere));
        return (int32_t)_proxies.size() - 1;
    } else {
        int32_t index = _freeIndices.back();
        _freeIndices.pop_back();
        _proxies[index].sphere = newSphere;
        _proxies[index].region = Space::REGION_UNKNOWN;
        _proxies[index].prevRegion = Space::REGION_UNKNOWN;
        return index;
    }
}

void Space::deleteProxy(int32_t proxyId) {
    if (proxyId >= (int32_t)_proxies.size() || _proxies.empty()) {
        return;
    }
    if (proxyId == (int32_t)_proxies.size() - 1) {
        // remove proxy on back
        _proxies.pop_back();
        if (!_freeIndices.empty()) {
            // remove any freeIndices on back
            std::sort(_freeIndices.begin(), _freeIndices.end());
            while(!_freeIndices.empty() && _freeIndices.back() == (int32_t)_proxies.size() - 1) {
                _freeIndices.pop_back();
                _proxies.pop_back();
            }
        }
    } else {
        _proxies[proxyId].region = Space::REGION_INVALID;
        _freeIndices.push_back(proxyId);
    }
}

void Space::updateProxy(int32_t proxyId, const Space::Sphere& newSphere) {
    if (proxyId >= (int32_t)_proxies.size()) {
        return;
    }
    _proxies[proxyId].sphere = newSphere;
}

void Space::setViews(const std::vector<Space::View>& views) {
    _views = views;
}

void Space::categorizeAndGetChanges(std::vector<Space::Change>& changes) {
    uint32_t numProxies = (uint32_t)_proxies.size();
    uint32_t numViews = (uint32_t)_views.size();
    for (uint32_t i = 0; i < numProxies; ++i) {
        Proxy& proxy = _proxies[i];
        if (proxy.region < Space::REGION_INVALID) {
            uint8_t region = Space::REGION_UNKNOWN;
            for (uint32_t j = 0; j < numViews; ++j) {
                float distance2 = glm::distance2(_views[j].center, glm::vec3(_proxies[i].sphere));
                for (uint8_t c = 0; c < region; ++c) {
                    float touchDistance = _views[j].radiuses[c] + _proxies[i].sphere.w;
                    if (distance2 < touchDistance * touchDistance) {
                        region = c;
                        break;
                    }
                }
            }
            proxy.prevRegion = proxy.region;
            proxy.region = region;
            if (proxy.region != proxy.prevRegion) {
                changes.emplace_back(Space::Change((int32_t)i, proxy.region, proxy.prevRegion));
            }
        }
    }
}

