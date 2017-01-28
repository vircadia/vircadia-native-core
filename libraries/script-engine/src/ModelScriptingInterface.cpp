//
//  ModelScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelScriptingInterface.h"

ModelScriptingInterface::ModelScriptingInterface(QObject* parent) : QObject(parent) {
}


MeshProxy::MeshProxy(MeshPointer mesh) : _mesh(mesh) {
}

MeshProxy::~MeshProxy() {
}


QScriptValue meshToScriptValue(QScriptEngine* engine, MeshProxy* const &in) {
    QScriptValue obj("something");
    return obj;
}

void meshFromScriptValue(const QScriptValue& value, MeshProxy* &out) {
}
