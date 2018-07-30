//
//  Created by Sam Gondelman 10/19/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PickManager.h"

PickManager::PickManager() {
    setShouldPickHUDOperator([]() { return false; });
    setCalculatePos2DFromHUDOperator([](const glm::vec3& intersection) { return glm::vec2(NAN); });
}

unsigned int PickManager::addPick(PickQuery::PickType type, const std::shared_ptr<PickQuery> pick) {
    unsigned int id = INVALID_PICK_ID;
    withWriteLock([&] {
        // Don't let the pick IDs overflow
        if (_nextPickID < UINT32_MAX) {
            id = _nextPickID++;
            _picks[type][id] = pick;
            _typeMap[id] = type;
        }
    });
    return id;
}

std::shared_ptr<PickQuery> PickManager::findPick(unsigned int uid) const {
    return resultWithReadLock<std::shared_ptr<PickQuery>>([&] {
        auto type = _typeMap.find(uid);
        if (type != _typeMap.end()) {
            return _picks.find(type->second)->second.find(uid)->second;
        }
        return std::shared_ptr<PickQuery>();
    });
}

void PickManager::removePick(unsigned int uid) {
    withWriteLock([&] {
        auto type = _typeMap.find(uid);
        if (type != _typeMap.end()) {
            _picks[type->second].erase(uid);
            _typeMap.erase(uid);
        }
    });
}

PickResultPointer PickManager::getPrevPickResult(unsigned int uid) const {
    auto pick = findPick(uid);
    if (pick) {
        return pick->getPrevPickResult();
    }
    return PickResultPointer();
}

void PickManager::enablePick(unsigned int uid) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->enable();
    }
}

void PickManager::disablePick(unsigned int uid) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->disable();
    }
}

void PickManager::setPrecisionPicking(unsigned int uid, bool precisionPicking) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setPrecisionPicking(precisionPicking);
    }
}

void PickManager::setIgnoreItems(unsigned int uid, const QVector<QUuid>& ignore) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setIgnoreItems(ignore);
    }
}

void PickManager::setIncludeItems(unsigned int uid, const QVector<QUuid>& include) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setIncludeItems(include);
    }
}

void PickManager::update() {
    uint64_t expiry = usecTimestampNow() + _perFrameTimeBudget;
    std::unordered_map<PickQuery::PickType, std::unordered_map<unsigned int, std::shared_ptr<PickQuery>>> cachedPicks;
    withReadLock([&] {
        cachedPicks = _picks;
    });

    bool shouldPickHUD = _shouldPickHUDOperator();
    // we pass the same expiry to both updates, but the stylus updates are relatively cheap
    // and the rayPicks updae will ALWAYS update at least one ray even when there is no budget
    _stylusPickCacheOptimizer.update(cachedPicks[PickQuery::Stylus], _nextPickToUpdate[PickQuery::Stylus], expiry, false);
    _rayPickCacheOptimizer.update(cachedPicks[PickQuery::Ray], _nextPickToUpdate[PickQuery::Ray], expiry, shouldPickHUD);
    _parabolaPickCacheOptimizer.update(cachedPicks[PickQuery::Parabola], _nextPickToUpdate[PickQuery::Parabola], expiry, shouldPickHUD);
}

bool PickManager::isLeftHand(unsigned int uid) {
    auto pick = findPick(uid);
    if (pick) {
        return pick->isLeftHand();
    }
    return false;
}

bool PickManager::isRightHand(unsigned int uid) {
    auto pick = findPick(uid);
    if (pick) {
        return pick->isRightHand();
    }
    return false;
}

bool PickManager::isMouse(unsigned int uid) {
    auto pick = findPick(uid);
    if (pick) {
        return pick->isMouse();
    }
    return false;
}
