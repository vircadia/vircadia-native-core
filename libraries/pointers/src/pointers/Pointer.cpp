//
//  Created by Sam Gondelman 10/19/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Pointer.h"

#include <DependencyManager.h>
#include "PickManager.h"

Pointer::~Pointer() {
    DependencyManager::get<PickManager>()->removePick(_pickUID);
}

void Pointer::enable() {
    DependencyManager::get<PickManager>()->enablePick(_pickUID);
}

void Pointer::disable() {
    DependencyManager::get<PickManager>()->disablePick(_pickUID);
}

const QVariantMap Pointer::getPrevPickResult() {
    return DependencyManager::get<PickManager>()->getPrevPickResult(_pickUID);
}

void Pointer::setPrecisionPicking(const bool precisionPicking) {
    DependencyManager::get<PickManager>()->setPrecisionPicking(_pickUID, precisionPicking);
}

void Pointer::setIgnoreItems(const QVector<QUuid>& ignoreItems) const {
    DependencyManager::get<PickManager>()->setIgnoreItems(_pickUID, ignoreItems);
}

void Pointer::setIncludeItems(const QVector<QUuid>& includeItems) const {
    DependencyManager::get<PickManager>()->setIncludeItems(_pickUID, includeItems);
}