//
//  TemporaryPairwiseCollisionFilter.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2019.08.12
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TemporaryPairwiseCollisionFilter_h
#define hifi_TemporaryPairwiseCollisionFilter_h

#include <unordered_map>
#include <btBulletDynamicsCommon.h>

class TemporaryPairwiseCollisionFilter {
public:
    using LastContactMap = std::unordered_map<const btCollisionObject*, uint32_t>;

    TemporaryPairwiseCollisionFilter() { }

    bool isFiltered(const btCollisionObject* object) const;
    void incrementEntry(const btCollisionObject* object);
    void expireOldEntries();
    void clearAllEntries() { _filteredContacts.clear(); _stepCount = 0; }
    void incrementStepCount() { ++_stepCount; }

protected:
    LastContactMap _filteredContacts;
    uint32_t _stepCount { 0 };
};

#endif // hifi_TemporaryPairwiseCollisionFilter_h
