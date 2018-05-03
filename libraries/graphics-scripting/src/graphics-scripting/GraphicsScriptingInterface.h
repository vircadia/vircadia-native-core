//
//  GraphicsScriptingInterface.h
//  libraries/graphics-scripting/src
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GraphicsScriptingInterface_h
#define hifi_GraphicsScriptingInterface_h

#include <QtCore/QObject>
#include <QUrl>

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptable>

#include "ScriptableMesh.h"
#include <DependencyManager.h>


/**jsdoc
 * The experimental Graphics API <em>(experimental)</em> lets you query and manage certain graphics-related structures (like underlying meshes and textures) from scripting.
 * @namespace Graphics
 *
 * @hifi-interface
 * @hifi-client-entity
 */

class GraphicsScriptingInterface : public QObject, public QScriptable, public Dependency {
    Q_OBJECT

public:
    static void registerMetaTypes(QScriptEngine* engine);
    GraphicsScriptingInterface(QObject* parent = nullptr);

public slots:
    /**jsdoc
     * Returns a model reference object associated with the specified UUID ({@link EntityID}, {@link OverlayID}, or {@link AvatarID}).
     *
     * @function Graphics.getModel
     * @param {UUID} entityID - The objectID of the model whose meshes are to be retrieved.
     * @returns {Graphics.Model} the resulting Model object
     */
    scriptable::ScriptableModelPointer getModel(QUuid uuid);

    bool updateModel(QUuid uuid, const scriptable::ScriptableModelPointer& model);

    bool canUpdateModel(QUuid uuid, int meshIndex = -1, int partNumber = -1);

    scriptable::ScriptableModelPointer newModel(const scriptable::ScriptableMeshes& meshes);

    /**jsdoc
     * Create a new Mesh / Mesh Part with the specified data buffers.
     *
     * @function Graphics.newMesh
     * @param {Graphics.IFSData} ifsMeshData Index-Faced Set (IFS) arrays used to create the new mesh.
     * @returns {Graphics.Mesh} the resulting Mesh / Mesh Part object
     */
    /**jsdoc
    * @typedef {object} Graphics.IFSData
    * @property {string} [name] - mesh name (useful for debugging / debug prints).
    * @property {number[]} indices - vertex indices to use for the mesh faces.
    * @property {Vec3[]} vertices - vertex positions (model space)
    * @property {Vec3[]} [normals] - vertex normals (normalized)
    * @property {Vec3[]} [colors] - vertex colors (normalized)
    * @property {Vec2[]} [texCoords0] - vertex texture coordinates (normalized)
    */
    scriptable::ScriptableMeshPointer newMesh(const QVariantMap& ifsMeshData);

#ifdef SCRIPTABLE_MESH_TODO
    scriptable::ScriptableMeshPartPointer exportMeshPart(scriptable::ScriptableMeshPointer mesh, int partNumber = -1) {
        return scriptable::make_scriptowned<scriptable::ScriptableMeshPart>(mesh, part);
    }
    bool updateMeshPart(scriptable::ScriptableMeshPointer mesh, scriptable::ScriptableMeshPartPointer part);
#endif

    QString exportModelToOBJ(const scriptable::ScriptableModel& in);

private:
    scriptable::ModelProviderPointer getModelProvider(QUuid uuid);
    void jsThrowError(const QString& error);
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMeshPointer meshProxy);
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMesh& meshProxy);
    scriptable::MeshPointer getMeshPointer(const scriptable::ScriptableMesh& meshProxy);

};

Q_DECLARE_METATYPE(glm::uint32)
Q_DECLARE_METATYPE(QVector<glm::uint32>)
Q_DECLARE_METATYPE(NestableType)

#endif // hifi_GraphicsScriptingInterface_h
