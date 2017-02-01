//
//  Created by Ryan Huffman on 2016-12-14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StatTracker.h"

StatTracker::StatTracker() {
    
}

QVariant StatTracker::getStat(const QString& name) {
    std::lock_guard<std::mutex> lock(_statsLock);
    return _stats[name];
}

void StatTracker::setStat(const QString& name, int value) {
    Lock lock(_statsLock);
    _stats[name] = value;
}

void StatTracker::updateStat(const QString& name, int value) {
    Lock lock(_statsLock);
    auto itr = _stats.find(name);
    if (_stats.end() == itr) {
        _stats[name] = value;
    } else {
        *itr = *itr + value;
    }
}

void StatTracker::incrementStat(const QString& name) {
    updateStat(name, 1);
}

void StatTracker::decrementStat(const QString& name) {
    updateStat(name, -1);
}