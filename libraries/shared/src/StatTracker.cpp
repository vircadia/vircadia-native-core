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

QVariant StatTracker::getStat(QString name) {
    std::lock_guard<std::mutex> lock(_statsLock);
    return _stats[name];
}

void StatTracker::editStat(QString name, EditStatFunction fn) {
    std::lock_guard<std::mutex> lock(_statsLock);
    _stats[name] = fn(_stats[name]);
}

void StatTracker::incrementStat(QString name) {
    std::lock_guard<std::mutex> lock(_statsLock);
    QVariant stat = _stats[name];
    _stats[name] = _stats[name].toInt() + 1;
}

void StatTracker::decrementStat(QString name) {
    std::lock_guard<std::mutex> lock(_statsLock);
    _stats[name] = _stats[name].toInt() - 1;
}