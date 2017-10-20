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
}

QUuid PickManager::addPick(PickQuery::PickType type, const std::shared_ptr<PickQuery> pick) {
    QUuid id = QUuid::createUuid();
    withWriteLock([&] {
        _picks[type][id] = pick;
        _typeMap[id] = type;
    });
    return id;
}

std::shared_ptr<PickQuery> PickManager::findPick(const QUuid& uid) const {
    return resultWithReadLock<std::shared_ptr<PickQuery>>([&] {
        auto type = _typeMap.find(uid);
        if (type != _typeMap.end()) {
            return _picks[type.value()][uid];
        }
        return std::shared_ptr<PickQuery>();
    });
}

void PickManager::removePick(const QUuid& uid) {
    withWriteLock([&] {
        auto type = _typeMap.find(uid);
        if (type != _typeMap.end()) {
            _picks[type.value()].remove(uid);
            _typeMap.remove(uid);
        }
    });
}

QVariantMap PickManager::getPrevPickResult(const QUuid& uid) const {
    auto pick = findPick(uid);
    if (pick && pick->getPrevPickResult()) {
        return pick->getPrevPickResult()->toVariantMap();
    }
    return QVariantMap();
}

void PickManager::enablePick(const QUuid& uid) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->enable();
    }
}

void PickManager::disablePick(const QUuid& uid) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->disable();
    }
}

void PickManager::setPrecisionPicking(const QUuid& uid, bool precisionPicking) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setPrecisionPicking(precisionPicking);
    }
}

void PickManager::setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignore) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setIgnoreItems(ignore);
    }
}

void PickManager::setIncludeItems(const QUuid& uid, const QVector<QUuid>& include) const {
    auto pick = findPick(uid);
    if (pick) {
        pick->setIncludeItems(include);
    }
}

void PickManager::update() {
    QHash<PickQuery::PickType, QHash<QUuid, std::shared_ptr<PickQuery>>> cachedPicks;
    withReadLock([&] {
        cachedPicks = _picks;
    });

    bool shouldPickHUD = _shouldPickHUDOperator();
    _rayPickCacheOptimizer.update(cachedPicks[PickQuery::Ray], shouldPickHUD);
}