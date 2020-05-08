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

class Space : public Collection {
public:
    using ProxyUpdate = std::pair<int32_t, Sphere>;

    class Change {
    public:
        Change(int32_t i, uint32_t c, uint32_t p) : proxyId(i), region(c), prevRegion(p) {}
        int32_t proxyId { -1 };
        uint8_t region { 0 };
        uint8_t prevRegion { 0 };
    };

    Space();

    void setViews(const Views& views);

    uint32_t getNumViews() const { return (uint32_t)(_views.size()); }
    void copyViews(std::vector<View>& copy) const;

    uint32_t getNumObjects() const { return _IDAllocator.getNumLiveIndices(); }
    uint32_t getNumAllocatedProxies() const { return (uint32_t)(_IDAllocator.getNumAllocatedIndices()); }

    void categorizeAndGetChanges(std::vector<Change>& changes);
    uint32_t copyProxyValues(Proxy* proxies, uint32_t numDestProxies) const;
    uint32_t copySelectedProxyValues(Proxy::Vector& proxies, const workload::indexed_container::Indices& indices) const;

    const Owner getOwner(int32_t proxyID) const;
    uint8_t getRegion(int32_t proxyID) const;

    void clear() override;
private:

    void processTransactionFrame(const Transaction& transaction) override;
    void processResets(const Transaction::Resets& transactions);
    void processRemoves(const Transaction::Removes& transactions);
    void processUpdates(const Transaction::Updates& transactions);

    // The database of proxies is protected for editing by a mutex
    mutable std::mutex _proxiesMutex;
    Proxy::Vector _proxies;
    std::vector<Owner> _owners;

    Views _views;
};

using SpacePointer = std::shared_ptr<Space>;
using Changes = std::vector<Space::Change>;
using IndexVectors = std::vector<IndexVector>;
using Timing_ns = std::chrono::nanoseconds;
using Timings = std::vector<Timing_ns>;

} // namespace workload

#endif // hifi_workload_Space_h
