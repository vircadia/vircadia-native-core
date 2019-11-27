//
//  TemporaryPairwiseCollisionFilter.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2019.08.12
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TemporaryPairwiseCollisionFilter.h"

bool TemporaryPairwiseCollisionFilter::isFiltered(const btCollisionObject* object) const {
    return _filteredContacts.find(object) != _filteredContacts.end();
}

void TemporaryPairwiseCollisionFilter::incrementEntry(const btCollisionObject* object) {
    LastContactMap::iterator itr = _filteredContacts.find(object);
    if (itr == _filteredContacts.end()) {
        _filteredContacts.emplace(std::make_pair(object, _stepCount));
    } else {
        itr->second = _stepCount;
    }
}

void TemporaryPairwiseCollisionFilter::expireOldEntries() {
    LastContactMap::iterator itr = _filteredContacts.begin();
    while (itr != _filteredContacts.end()) {
        if (itr->second < _stepCount) {
            itr = _filteredContacts.erase(itr);
        } else {
            ++itr;
        }
    }
}
