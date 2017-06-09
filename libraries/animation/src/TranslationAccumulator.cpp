//
//  TranslationAccumulator.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TranslationAccumulator.h"

void TranslationAccumulator::add(const glm::vec3& translation, float weight) {
    _accum += weight * translation;
    _totalWeight += weight;
    _isDirty = true;
}

glm::vec3 TranslationAccumulator::getAverage() {
    if (_totalWeight > 0.0f) {
        return _accum / _totalWeight;
    } else {
        return glm::vec3();
    }
}

void TranslationAccumulator::clear() {
    _accum *= 0.0f;
    _totalWeight = 0.0f;
}

void TranslationAccumulator::clearAndClean() {
    clear();
    _isDirty = false;
}
