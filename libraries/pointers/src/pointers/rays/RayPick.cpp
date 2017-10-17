//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RayPick.h"

const RayPickFilter RayPickFilter::NOTHING;

RayPick::RayPick(const RayPickFilter& filter, const float maxDistance, const bool enabled) :
    _filter(filter),
    _maxDistance(maxDistance),
    _enabled(enabled)
{
}

void RayPick::enable(bool enabled) {
    withWriteLock([&] {
        _enabled = enabled;
    });
}

RayPickFilter RayPick::getFilter() const { 
    return resultWithReadLock<RayPickFilter>([&] {
        return _filter;
    });
}

float RayPick::getMaxDistance() const { 
    return _maxDistance; 
}

bool RayPick::isEnabled() const {
    return resultWithReadLock<bool>([&] {
        return _enabled;
    });
}

void RayPick::setPrecisionPicking(bool precisionPicking) { 
    withWriteLock([&]{
        _filter.setFlag(RayPickFilter::PICK_COARSE, !precisionPicking);
    });
}

void RayPick::setRayPickResult(const RayPickResult& rayPickResult) {
    withWriteLock([&] {
        _prevResult = rayPickResult;
    });
}

QVector<QUuid> RayPick::getIgnoreItems() const {
    return resultWithReadLock<QVector<QUuid>>([&] {
        return _ignoreItems;
    });
}

QVector<QUuid> RayPick::getIncludeItems() const {
    return resultWithReadLock<QVector<QUuid>>([&] {
        return _includeItems;
    });
}

RayPickResult RayPick::getPrevRayPickResult() const {
    return resultWithReadLock<RayPickResult>([&] {
        return _prevResult;
    });
}

void RayPick::setIgnoreItems(const QVector<QUuid>& ignoreItems) {
    withWriteLock([&] {
        _ignoreItems = ignoreItems;
    });
}

void RayPick::setIncludeItems(const QVector<QUuid>& includeItems) {
    withWriteLock([&] {
        _includeItems = includeItems;
    });
}
