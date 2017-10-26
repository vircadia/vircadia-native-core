//
//  RayPickScriptingInterface.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 8/15/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RayPickScriptingInterface.h"

#include <QVariant>
#include "GLMHelpers.h"

#include <pointers/PickManager.h>

#include "StaticRayPick.h"
#include "JointRayPick.h"
#include "MouseRayPick.h"

QUuid RayPickScriptingInterface::createRayPick(const QVariant& properties) {
    return DependencyManager::get<PickScriptingInterface>()->createRayPick(properties);
}

void RayPickScriptingInterface::enableRayPick(const QUuid& uid) {
    DependencyManager::get<PickManager>()->enablePick(uid);
}

void RayPickScriptingInterface::disableRayPick(const QUuid& uid) {
    DependencyManager::get<PickManager>()->disablePick(uid);
}

void RayPickScriptingInterface::removeRayPick(const QUuid& uid) {
    DependencyManager::get<PickManager>()->removePick(uid);
}

QVariantMap RayPickScriptingInterface::getPrevRayPickResult(const QUuid& uid) {
    return DependencyManager::get<PickManager>()->getPrevPickResult(uid);
}

void RayPickScriptingInterface::setPrecisionPicking(const QUuid& uid, const bool precisionPicking) {
    DependencyManager::get<PickManager>()->setPrecisionPicking(uid, precisionPicking);
}

void RayPickScriptingInterface::setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreItems) {
    DependencyManager::get<PickManager>()->setIgnoreItems(uid, qVectorQUuidFromScriptValue(ignoreItems));
}

void RayPickScriptingInterface::setIncludeItems(const QUuid& uid, const QScriptValue& includeItems) {
    DependencyManager::get<PickManager>()->setIncludeItems(uid, qVectorQUuidFromScriptValue(includeItems));
}
