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

#include <RegisteredMetaTypes.h>

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptable>

#include "ScriptableMesh.h"
#include <DependencyManager.h>
class ModelScriptingInterface : public QObject, public QScriptable, public Dependency {
    Q_OBJECT

public:
    ModelScriptingInterface(QObject* parent = nullptr);
    static void registerMetaTypes(QScriptEngine* engine);

public slots:
    /**jsdoc
     * Returns the meshes associated with a UUID (entityID, overlayID, or avatarID)
     *
     * @function ModelScriptingInterface.getMeshes
     * @param {EntityID} entityID The ID of the entity whose meshes are to be retrieve
     */
    void getMeshes(QUuid uuid, QScriptValue scopeOrCallback, QScriptValue methodOrName = QScriptValue());

    bool dedupeVertices(scriptable::ScriptableMeshPointer meshProxy, float epsilon = 1e-6);
    bool recalculateNormals(scriptable::ScriptableMeshPointer meshProxy);
    QScriptValue cloneMesh(scriptable::ScriptableMeshPointer meshProxy, bool recalcNormals = true);
    QScriptValue unrollVertices(scriptable::ScriptableMeshPointer meshProxy, bool recalcNormals = true);
    QScriptValue mapAttributeValues(QScriptValue in,
                                                QScriptValue scopeOrCallback,
                                                QScriptValue methodOrName = QScriptValue());
    QScriptValue mapMeshAttributeValues(scriptable::ScriptableMeshPointer meshProxy,
                                                QScriptValue scopeOrCallback,
                                                QScriptValue methodOrName = QScriptValue());

    QString meshToOBJ(const scriptable::ScriptableModel& in);

    bool replaceMeshData(scriptable::ScriptableMeshPointer dest, scriptable::ScriptableMeshPointer source, const QVector<QString>& attributeNames = QVector<QString>());
    QScriptValue appendMeshes(scriptable::ScriptableModel in);
    QScriptValue transformMesh(scriptable::ScriptableMeshPointer meshProxy, glm::mat4 transform);
    QScriptValue newMesh(const QVector<glm::vec3>& vertices,
                                     const QVector<glm::vec3>& normals,
                                     const QVector<mesh::MeshFace>& faces);
    QScriptValue getVertexCount(scriptable::ScriptableMeshPointer meshProxy);
    QScriptValue getVertex(scriptable::ScriptableMeshPointer meshProxy, mesh::uint32 vertexIndex);

private:
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMeshPointer meshProxy);

};

#endif // hifi_ModelScriptingInterface_h
