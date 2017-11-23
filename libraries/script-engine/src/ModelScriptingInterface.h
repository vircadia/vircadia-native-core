//
//  ModelScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelScriptingInterface_h
#define hifi_ModelScriptingInterface_h

#include <QtCore/QObject>

#include <RegisteredMetaTypes.h>
class QScriptEngine;

class ModelScriptingInterface : public QObject {
    Q_OBJECT

public:
    ModelScriptingInterface(QObject* parent);

    Q_INVOKABLE QString meshToOBJ(MeshProxyList in);
    Q_INVOKABLE QScriptValue appendMeshes(MeshProxyList in);
    Q_INVOKABLE QScriptValue transformMesh(glm::mat4 transform, MeshProxy* meshProxy);
    Q_INVOKABLE QScriptValue newMesh(const QVector<glm::vec3>& vertices,
                                     const QVector<glm::vec3>& normals,
                                     const QVector<MeshFace>& faces);
    Q_INVOKABLE QScriptValue getVertexCount(MeshProxy* meshProxy);
    Q_INVOKABLE QScriptValue getVertex(MeshProxy* meshProxy, int vertexIndex);

private:
    QScriptEngine* _modelScriptEngine { nullptr };
};

#endif // hifi_ModelScriptingInterface_h
