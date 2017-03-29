//
//  MeshFace.h
//  libraries/model/src/model/
//
//  Created by Seth Alves on 2017-3-23
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MeshFace_h
#define hifi_MeshFace_h

#include <QScriptEngine>
#include <QScriptValueIterator>
#include <QtScript/QScriptValue>

#include <model/Geometry.h>

using MeshPointer = std::shared_ptr<model::Mesh>;

class MeshFace {

public:
    MeshFace() {}
    ~MeshFace() {}

    QVector<uint32_t> vertexIndices;
    // TODO -- material...
};

Q_DECLARE_METATYPE(MeshFace)
Q_DECLARE_METATYPE(QVector<MeshFace>)

QScriptValue meshFaceToScriptValue(QScriptEngine* engine, const MeshFace &meshFace);
void meshFaceFromScriptValue(const QScriptValue &object, MeshFace& meshFaceResult);
QScriptValue qVectorMeshFaceToScriptValue(QScriptEngine* engine, const QVector<MeshFace>& vector);
void qVectorMeshFaceFromScriptValue(const QScriptValue& array, QVector<MeshFace>& result);



#endif // hifi_MeshFace_h
