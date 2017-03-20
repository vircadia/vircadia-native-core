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
#include <QScriptValueIterator>
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

QScriptValue meshesToScriptValue(QScriptEngine* engine, const MeshProxyList &in) {
    return engine->toScriptValue(in);
}

void meshesFromScriptValue(const QScriptValue& value, MeshProxyList &out) {
    QScriptValueIterator itr(value);
    while(itr.hasNext()) {
        itr.next();
        MeshProxy* meshProxy = qscriptvalue_cast<MeshProxyList::value_type>(itr.value());
        if (meshProxy) {
            out.append(meshProxy);
        }
    }
}

QString ModelScriptingInterface::meshToOBJ(MeshProxyList in) {
    QList<MeshPointer> meshes;
    foreach (const MeshProxy* meshProxy, in) {
        meshes.append(meshProxy->getMeshPointer());
    }

    return writeOBJToString(meshes);
}
