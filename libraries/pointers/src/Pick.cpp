//
//  Created by Sam Gondelman 10/17/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Pick.h"

int pickTypeMetaTypeId = qRegisterMetaType<PickQuery::PickType>("PickType");

PickQuery::PickQuery(const PickFilter& filter, const float maxDistance, const bool enabled) :
    _filter(filter),
    _maxDistance(maxDistance),
    _enabled(enabled) {
}

void PickQuery::enable(bool enabled) {
    withWriteLock([&] {
        _enabled = enabled;
    });
}

PickFilter PickQuery::getFilter() const {
    return resultWithReadLock<PickFilter>([&] {
        return _filter;
    });
}

float PickQuery::getMaxDistance() const {
    return _maxDistance;
}

bool PickQuery::isEnabled() const {
    return resultWithReadLock<bool>([&] {
        return _enabled;
    });
}

void PickQuery::setPrecisionPicking(bool precisionPicking) {
    withWriteLock([&] {
        _filter.setFlag(PickFilter::PRECISE, precisionPicking);
        _filter.setFlag(PickFilter::COARSE, !precisionPicking);
    });
}

void PickQuery::setPickResult(const PickResultPointer& pickResult) {
    withWriteLock([&] {
        _prevResult = pickResult;
    });
}

QVector<QUuid> PickQuery::getIgnoreItems() const {
    return resultWithReadLock<QVector<QUuid>>([&] {
        return _ignoreItems;
    });
}

void PickQuery::setScriptParameters(const QVariantMap& parameters) {
    _scriptParameters = parameters;
}

QVariantMap PickQuery::getScriptParameters() const {
    return _scriptParameters;
}

QVector<QUuid> PickQuery::getIncludeItems() const {
    return resultWithReadLock<QVector<QUuid>>([&] {
        return _includeItems;
    });
}

PickResultPointer PickQuery::getPrevPickResult() const {
    return resultWithReadLock<PickResultPointer>([&] {
        return _prevResult;
    });
}

void PickQuery::setIgnoreItems(const QVector<QUuid>& ignoreItems) {
    withWriteLock([&] {
        _ignoreItems = ignoreItems;
        // We sort these items here so the PickCacheOptimizer can catch cases where two picks have the same ignoreItems in a different order
        std::sort(_ignoreItems.begin(), _ignoreItems.end(), std::less<QUuid>());
    });
}

void PickQuery::setIncludeItems(const QVector<QUuid>& includeItems) {
    withWriteLock([&] {
        _includeItems = includeItems;
        // We sort these items here so the PickCacheOptimizer can catch cases where two picks have the same includeItems in a different order
        std::sort(_includeItems.begin(), _includeItems.end(), std::less<QUuid>());
    });
}