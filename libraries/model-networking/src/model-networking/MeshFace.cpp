//
//  MeshFace.cpp
//  libraries/model/src/model/
//
//  Created by Seth Alves on 2017-3-23
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <RegisteredMetaTypes.h>

#include "MeshFace.h"


QScriptValue meshFaceToScriptValue(QScriptEngine* engine, const MeshFace &meshFace) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("vertices", qVectorIntToScriptValue(engine, meshFace.vertexIndices));
    return obj;
}

void meshFaceFromScriptValue(const QScriptValue &object, MeshFace& meshFaceResult) {
    qVectorIntFromScriptValue(object.property("vertices"), meshFaceResult.vertexIndices);
}

QScriptValue qVectorMeshFaceToScriptValue(QScriptEngine* engine, const QVector<MeshFace>& vector) {
    QScriptValue array = engine->newArray();
    for (int i = 0; i < vector.size(); i++) {
        array.setProperty(i, meshFaceToScriptValue(engine, vector.at(i)));
    }
    return array;
}

void qVectorMeshFaceFromScriptValue(const QScriptValue& array, QVector<MeshFace>& result) {
    int length = array.property("length").toInteger();
    result.clear();

    for (int i = 0; i < length; i++) {
        MeshFace meshFace = MeshFace();
        meshFaceFromScriptValue(array.property(i), meshFace);
        result << meshFace;
    }
}
