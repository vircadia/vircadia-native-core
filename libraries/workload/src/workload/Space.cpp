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
#include <cstring>
#include <algorithm>

#include <glm/gtx/quaternion.hpp>

using namespace workload;

void Space::clear() {
    _proxies.clear();
    _freeIndices.clear();
}

int32_t Space::createProxy(const Space::Sphere& newSphere) {
    if (_freeIndices.empty()) {
        _proxies.emplace_back(Space::Proxy(newSphere));
        return (int32_t)_proxies.size() - 1;
    } else {
        int32_t index = _freeIndices.back();
        _freeIndices.pop_back();
        _proxies[index].sphere = newSphere;
        _proxies[index].region = Region::UNKNOWN;
        _proxies[index].prevRegion = Region::UNKNOWN;
        return index;
    }
}

void Space::deleteProxies(const std::vector<int32_t>& deadIndices) {
    for (uint32_t i = 0; i < deadIndices.size(); ++i) {
        deleteProxy(deadIndices[i]);
    }
}

void Space::updateProxies(const std::vector<ProxyUpdate>& changedProxies) {
    for (uint32_t i = 0; i < changedProxies.size(); ++i) {
        int32_t proxyId = changedProxies[i].first;
        if (proxyId > -1 && proxyId < (int32_t)_proxies.size()) {
            updateProxy(changedProxies[i].first, changedProxies[i].second);
        }
    }
}

void Space::setViews(const Views& views) {
    _views = views;
}

void Space::copyViews(std::vector<View>& copy) const {
    copy = _views;
}

void Space::categorizeAndGetChanges(std::vector<Space::Change>& changes) {
    uint32_t numProxies = (uint32_t)_proxies.size();
    uint32_t numViews = (uint32_t)_views.size();
    for (uint32_t i = 0; i < numProxies; ++i) {
        Proxy& proxy = _proxies[i];
        if (proxy.region < Region::INVALID) {
            glm::vec3 proxyCenter = glm::vec3(proxy.sphere);
            float proxyRadius = proxy.sphere.w;
            uint8_t region = Region::UNKNOWN;
            for (uint32_t j = 0; j < numViews; ++j) {
                auto& view = _views[j];
                // for each 'view' we need only increment 'k' below the current value of 'region'
                for (uint8_t k = 0; k < region; ++k) {
                    float touchDistance = proxyRadius + view.regions[k].w;
                    if (distance2(proxyCenter, glm::vec3(view.regions[k])) < touchDistance * touchDistance) {
                        region = k;
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

// private
void Space::deleteProxy(int32_t proxyId) {
    if (proxyId > -1 && proxyId < (int32_t)_proxies.size()) {
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
            _proxies[proxyId].region = Region::INVALID;
            _freeIndices.push_back(proxyId);
        }
    }
}

uint32_t Space::copyProxyValues(Proxy* proxies, uint32_t numDestProxies) const {
    auto numCopied = std::min(numDestProxies, (uint32_t)_proxies.size());
    memcpy(proxies, _proxies.data(), numCopied * sizeof(Proxy));
    return numCopied;
}


// private
void Space::updateProxy(int32_t proxyId, const Space::Sphere& newSphere) {
    if (proxyId > -1 && proxyId < (int32_t)_proxies.size()) {
        _proxies[proxyId].sphere = newSphere;
    }
}

