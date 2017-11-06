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

uint32_t RayPickScriptingInterface::createRayPick(const QVariant& properties) {
    return DependencyManager::get<PickScriptingInterface>()->createRayPick(properties);
}

void RayPickScriptingInterface::enableRayPick(uint32_t uid) {
    DependencyManager::get<PickManager>()->enablePick(uid);
}

void RayPickScriptingInterface::disableRayPick(uint32_t uid) {
    DependencyManager::get<PickManager>()->disablePick(uid);
}

void RayPickScriptingInterface::removeRayPick(uint32_t uid) {
    DependencyManager::get<PickManager>()->removePick(uid);
}

QVariantMap RayPickScriptingInterface::getPrevRayPickResult(uint32_t uid) {
    return DependencyManager::get<PickManager>()->getPrevPickResult(uid);
}

void RayPickScriptingInterface::setPrecisionPicking(uint32_t uid, const bool precisionPicking) {
    DependencyManager::get<PickManager>()->setPrecisionPicking(uid, precisionPicking);
}

void RayPickScriptingInterface::setIgnoreItems(uint32_t uid, const QScriptValue& ignoreItems) {
    DependencyManager::get<PickManager>()->setIgnoreItems(uid, qVectorQUuidFromScriptValue(ignoreItems));
}

void RayPickScriptingInterface::setIncludeItems(uint32_t uid, const QScriptValue& includeItems) {
    DependencyManager::get<PickManager>()->setIncludeItems(uid, qVectorQUuidFromScriptValue(includeItems));
}

bool RayPickScriptingInterface::isLeftHand(unsigned int uid) {
    return DependencyManager::get<PickManager>()->isLeftHand(uid);
}

bool RayPickScriptingInterface::isRightHand(unsigned int uid) {
    return DependencyManager::get<PickManager>()->isRightHand(uid);
}

bool RayPickScriptingInterface::isMouse(unsigned int uid) {
    return DependencyManager::get<PickManager>()->isMouse(uid);
}