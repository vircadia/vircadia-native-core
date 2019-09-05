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
#include "RegisteredMetaTypes.h"


/**jsdoc
 * The experimental Graphics API <em>(experimental)</em> lets you query and manage certain graphics-related structures (like underlying meshes and textures) from scripting.
 * @namespace Graphics
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */

class GraphicsScriptingInterface : public QObject, public QScriptable, public Dependency {
    Q_OBJECT

public:
    static void registerMetaTypes(QScriptEngine* engine);
    GraphicsScriptingInterface(QObject* parent = nullptr);

public slots:
    /**jsdoc
     * Returns a model reference object associated with the specified UUID ({@link EntityID} or {@link AvatarID}).
     *
     * @function Graphics.getModel
     * @param {UUID} entityID - The objectID of the model whose meshes are to be retrieved.
     * @returns {Graphics.Model} the resulting Model object
     */
    scriptable::ScriptableModelPointer getModel(const QUuid& uuid);

    /**jsdoc
     * @function Graphics.updateModel
     * @param {Uuid} id
     * @param {Graphics.Model} model
     * @returns {boolean}
     */
    bool updateModel(const QUuid& uuid, const scriptable::ScriptableModelPointer& model);

    /**jsdoc
     * @function Graphics.canUpdateModel
     * @param {Uuid} id
     * @param {number} [meshIndex=-1]
     * @param {number} [partNumber=-1]
     * @returns {boolean}
     */
    bool canUpdateModel(const QUuid& uuid, int meshIndex = -1, int partNumber = -1);

    /**jsdoc
     * @function Graphics.newModel
     * @param {Graphics.Mesh[]} meshes
     * @returns {Graphics.Model}
     */
    scriptable::ScriptableModelPointer newModel(const scriptable::ScriptableMeshes& meshes);

    /**jsdoc
     * Create a new Mesh / Mesh Part with the specified data buffers.
     *
     * @function Graphics.newMesh
     * @param {Graphics.IFSData} ifsMeshData Index-Faced Set (IFS) arrays used to create the new mesh.
     * @returns {Graphics.Mesh} the resulting Mesh / Mesh Part object
     */
    scriptable::ScriptableMeshPointer newMesh(const QVariantMap& ifsMeshData);

#ifdef SCRIPTABLE_MESH_TODO
    scriptable::ScriptableMeshPartPointer exportMeshPart(scriptable::ScriptableMeshPointer mesh, int partNumber = -1) {
        return scriptable::make_scriptowned<scriptable::ScriptableMeshPart>(mesh, part);
    }
    bool updateMeshPart(scriptable::ScriptableMeshPointer mesh, scriptable::ScriptableMeshPartPointer part);
#endif

    /**jsdoc
     * @function Graphics.exportModelToOBJ
     * @param {Graphics.Model} model
     * @returns {string}
     */
    QString exportModelToOBJ(const scriptable::ScriptableModel& in);

private:
    scriptable::ModelProviderPointer getModelProvider(const QUuid& uuid);
    void jsThrowError(const QString& error);
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMeshPointer meshProxy);
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMesh& meshProxy);
    scriptable::MeshPointer getMeshPointer(const scriptable::ScriptableMesh& meshProxy);

};

namespace scriptable {
    QScriptValue scriptableMaterialToScriptValue(QScriptEngine* engine, const scriptable::ScriptableMaterial &material);
};

Q_DECLARE_METATYPE(NestableType)

#endif // hifi_GraphicsScriptingInterface_h
