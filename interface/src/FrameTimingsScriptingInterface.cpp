//
//  Created by Bradley Austin Davis on 2016/04/04
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FrameTimingsScriptingInterface.h"

#include <TextureCache.h>

void FrameTimingsScriptingInterface::start() {
    _values.clear();
    DependencyManager::get<TextureCache>()->setUnusedResourceCacheSize(0);
    _values.reserve(8192);
    _active = true;
}

void FrameTimingsScriptingInterface::addValue(uint64_t value) {
    if (_active) {
        _values.push_back(value);
    }
}

void FrameTimingsScriptingInterface::finish() {
    _active = false;
    uint64_t total = 0;
    _min = std::numeric_limits<uint64_t>::max();
    _max = std::numeric_limits<uint64_t>::lowest();
    size_t count = _values.size();
    for (size_t i = 0; i < count; ++i) {
        const uint64_t& value = _values[i];
        _max = std::max(_max, value);
        _min = std::min(_min, value);
        total += value;
    }
    _mean = (float)total / (float)count;
    float deviationTotal = 0;
    for (size_t i = 0; i < count; ++i) {
        float deviation = _values[i] - _mean;
        deviationTotal += deviation*deviation;
    }
    _stdDev = sqrt(deviationTotal / (float)count);
}

QVariantList FrameTimingsScriptingInterface::getValues() const {
    QVariantList result;
    for (quint64 v : _values) {
        result << QVariant(v);
    }
    return result;
}
