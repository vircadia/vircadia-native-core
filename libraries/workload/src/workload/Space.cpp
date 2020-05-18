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

Space::Space() : Collection() {
}

void Space::processTransactionFrame(const Transaction& transaction) {
    std::unique_lock<std::mutex> lock(_proxiesMutex);
    // Here we should be able to check the value of last ProxyID allocated
    // and allocate new proxies accordingly
    ProxyID maxID = _IDAllocator.getNumAllocatedIndices();
    if (maxID > (Index) _proxies.size()) {
        _proxies.resize(maxID + 100); // allocate the maxId and more
        _owners.resize(maxID + 100);
    }
    // Now we know for sure that we have enough items in the array to
    // capture anything coming from the transaction

    processResets(transaction._resetItems);
    processUpdates(transaction._updatedItems);
    processRemoves(transaction._removedItems);
}

void Space::processResets(const Transaction::Resets& transactions) {
    for (auto& reset : transactions) {
        // Access the true item
        auto proxyID = std::get<0>(reset);

        // Guard against proxyID being past the end of the list.
        if (!_IDAllocator.checkIndex(proxyID)) {
            continue;
        }
        auto& item = _proxies[proxyID];

        // Reset the item with a new payload
        item.sphere = (std::get<1>(reset));
        item.prevRegion = item.region = Region::UNKNOWN;

        _owners[proxyID] = (std::get<2>(reset));
    }
}

void Space::processRemoves(const Transaction::Removes& transactions) {
    for (auto removedID : transactions) {
        if (!_IDAllocator.checkIndex(removedID)) {
            continue;
        }
        _IDAllocator.freeIndex(removedID);

        // Access the true item
        auto& item = _proxies[removedID];

        // Kill it
        item.prevRegion = item.region = Region::INVALID;
        _owners[removedID] = Owner();
    }
}

void Space::processUpdates(const Transaction::Updates& transactions) {
    for (auto& update : transactions) {
        auto updateID = std::get<0>(update);
        if (!_IDAllocator.checkIndex(updateID)) {
            continue;
        }

        // Access the true item
        auto& item = _proxies[updateID];

        // Update the item
        item.sphere = (std::get<1>(update));
    }
}

void Space::categorizeAndGetChanges(std::vector<Space::Change>& changes) {
    std::unique_lock<std::mutex> lock(_proxiesMutex);
    uint32_t numProxies = (uint32_t)_proxies.size();
    uint32_t numViews = (uint32_t)_views.size();
    for (uint32_t i = 0; i < numProxies; ++i) {
        Proxy& proxy = _proxies[i];
        if (proxy.region < Region::INVALID) {
            glm::vec3 proxyCenter = glm::vec3(proxy.sphere);
            float proxyRadius = proxy.sphere.w;
            uint8_t region = Region::R4;
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

uint32_t Space::copyProxyValues(Proxy* proxies, uint32_t numDestProxies) const {
    std::unique_lock<std::mutex> lock(_proxiesMutex);
    auto numCopied = std::min(numDestProxies, (uint32_t)_proxies.size());
    memcpy(proxies, _proxies.data(), numCopied * sizeof(Proxy));
    return numCopied;
}

uint32_t Space::copySelectedProxyValues(Proxy::Vector& proxies, const workload::indexed_container::Indices& indices) const {
    std::unique_lock<std::mutex> lock(_proxiesMutex);
    uint32_t numCopied = 0;
    for (auto index : indices) {
        if (isAllocatedID(index) && (index < (Index)_proxies.size())) {
            proxies.push_back(_proxies[index]);
            ++numCopied;
        }
    }
    return numCopied;
}

const Owner Space::getOwner(int32_t proxyID) const {
    std::unique_lock<std::mutex> lock(_proxiesMutex);
    if (isAllocatedID(proxyID) && (proxyID < (Index)_proxies.size())) {
        return _owners[proxyID];
    }
    return Owner();
}

uint8_t Space::getRegion(int32_t proxyID) const {
    std::unique_lock<std::mutex> lock(_proxiesMutex);
    if (isAllocatedID(proxyID) && (proxyID < (Index)_proxies.size())) {
        return _proxies[proxyID].region;
    }
    return (uint8_t)Region::INVALID;
}

void Space::clear() {
    Collection::clear();
    std::unique_lock<std::mutex> lock(_proxiesMutex);
    _IDAllocator.clear();
    _proxies.clear();
    _owners.clear();
    _views.clear();
}

void Space::setViews(const Views& views) {
    _views = views;
}

void Space::copyViews(std::vector<View>& copy) const {
    copy = _views;
}
