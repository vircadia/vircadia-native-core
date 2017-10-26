//
//  LaserPointerScriptingInterface.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LaserPointerScriptingInterface.h"

#include "RegisteredMetaTypes.h"
#include "PointerScriptingInterface.h"

void LaserPointerScriptingInterface::setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreItems) const { 
    DependencyManager::get<PointerManager>()->setIgnoreItems(uid, qVectorQUuidFromScriptValue(ignoreItems));
}
void LaserPointerScriptingInterface::setIncludeItems(const QUuid& uid, const QScriptValue& includeItems) const {
    DependencyManager::get<PointerManager>()->setIncludeItems(uid, qVectorQUuidFromScriptValue(includeItems));
}

QUuid LaserPointerScriptingInterface::createLaserPointer(const QVariant& properties) const {
    return DependencyManager::get<PointerScriptingInterface>()->createLaserPointer(properties);
}

void LaserPointerScriptingInterface::editRenderState(const QUuid& uid, const QString& renderState, const QVariant& properties) const {
    DependencyManager::get<PointerScriptingInterface>()->editRenderState(uid, renderState, properties);
}