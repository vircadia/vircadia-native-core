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

#include <QScriptEngine>
#include <QtScript/QScriptValue>
#include "ModelScriptingInterface.h"
#include "OBJWriter.h"


ModelScriptingInterface::ModelScriptingInterface(QObject* parent) : QObject(parent) {
}

QScriptValue meshToScriptValue(QScriptEngine* engine, MeshProxy* const &in) {
    return engine->newQObject(in, QScriptEngine::QtOwnership,
                              QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects);
}

void meshFromScriptValue(const QScriptValue& value, MeshProxy* &out) {
    out = qobject_cast<MeshProxy*>(value.toQObject());
}

QString ModelScriptingInterface::meshToOBJ(MeshProxy* const &in) {
    bool success;
    QString filename = "/tmp/okokok.obj";
    success = writeOBJToFile(filename, in->getMeshPointer());
    if (!success) {
        return "";
    }
    return filename;
}
