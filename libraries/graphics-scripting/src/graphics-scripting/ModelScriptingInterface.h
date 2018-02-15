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
#include <QUrl>

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptable>

#include "ScriptableMesh.h"
#include <DependencyManager.h>

class ModelScriptingInterface : public QObject, public QScriptable, public Dependency {
    Q_OBJECT

public:
    ModelScriptingInterface(QObject* parent = nullptr);

public slots:
    /**jsdoc
     * Returns the meshes associated with a UUID (entityID, overlayID, or avatarID)
     *
     * @function ModelScriptingInterface.getMeshes
     * @param {EntityID} entityID The ID of the entity whose meshes are to be retrieve
     */
    void getMeshes(QUuid uuid, QScriptValue callback);
    bool updateMeshes(QUuid uuid, const scriptable::ScriptableModelPointer model);
    bool updateMeshes(QUuid uuid, const scriptable::ScriptableMeshPointer mesh, int meshIndex=0, int partIndex=0);

    QString meshToOBJ(const scriptable::ScriptableModel& in);

    QScriptValue appendMeshes(scriptable::ScriptableModel in);
    QScriptValue transformMesh(scriptable::ScriptableMeshPointer meshProxy, glm::mat4 transform);
    QScriptValue newMesh(const QVector<glm::vec3>& vertices,
                                     const QVector<glm::vec3>& normals,
                                     const QVector<mesh::MeshFace>& faces);
    QScriptValue getVertexCount(scriptable::ScriptableMeshPointer meshProxy);
    QScriptValue getVertex(scriptable::ScriptableMeshPointer meshProxy, quint32 vertexIndex);

    static void registerMetaTypes(QScriptEngine* engine);
    
private:
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMeshPointer meshProxy);
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMesh& meshProxy);
    scriptable::MeshPointer getMeshPointer(const scriptable::ScriptableMesh& meshProxy);

};

#endif // hifi_ModelScriptingInterface_h
