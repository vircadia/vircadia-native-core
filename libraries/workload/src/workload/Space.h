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

#include "Transaction.h"

namespace workload {

class Space {
public:
    using ProxyUpdate = std::pair<int32_t, Sphere>;

    class Change {
    public:
        Change(int32_t i, uint32_t c, uint32_t p) : proxyId(i), region(c), prevRegion(p) {}
        int32_t proxyId { -1 };
        uint8_t region { 0 };
        uint8_t prevRegion { 0 };
    };

    Space() {}

    // This call is thread safe, can be called from anywhere to allocate a new ID
    ProxyID allocateID();

    // Check that the ID is valid and allocated for this space, this a threadsafe call
    bool isAllocatedID(const ProxyID& id) const;

    // THis is the total number of allocated proxies, this a threadsafe call
    Index getNumAllocatedProxies() const { return _numAllocatedProxies.load(); }

    // Enqueue transaction to the space
    void enqueueTransaction(const Transaction& transaction);

    // Enqueue transaction to the space
    void enqueueTransaction(Transaction&& transaction);

    // Enqueue end of frame transactions boundary
    uint32_t enqueueFrame();

    // Process the pending transactions queued
    void processTransactionQueue();

    void setViews(const std::vector<View>& views);

    uint32_t getNumViews() const { return (uint32_t)(_views.size()); }
    void copyViews(std::vector<View>& copy) const;


    uint32_t getNumObjects() const { return (uint32_t)(_proxies.size() - _freeIndices.size()); }
    uint32_t getNumAllocatedProxies() const { return (uint32_t)(_proxies.size()); }

    void categorizeAndGetChanges(std::vector<Change>& changes);
    uint32_t copyProxyValues(Proxy* proxies, uint32_t numDestProxies) const;

private:


    void clear();
    int32_t createProxy(const Sphere& sphere);
    void deleteProxies(const IndexVector& deadIndices);
    void updateProxies(const std::vector<ProxyUpdate>& changedProxies);

    void deleteProxy(int32_t proxyId);
    void updateProxy(int32_t proxyId, const Sphere& sphere);

    std::vector<Proxy> _proxies;
    Views _views;
    IndexVector _freeIndices;
};

using SpacePointer = std::shared_ptr<Space>;
using Changes = std::vector<Space::Change>;
using IndexVectors = std::vector<IndexVector>;

} // namespace workload

#endif // hifi_workload_Space_h
