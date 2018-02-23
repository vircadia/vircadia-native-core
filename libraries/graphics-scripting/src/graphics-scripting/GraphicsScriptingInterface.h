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

class GraphicsScriptingInterface : public QObject, public QScriptable, public Dependency {
    Q_OBJECT

public:
    static void registerMetaTypes(QScriptEngine* engine);
    GraphicsScriptingInterface(QObject* parent = nullptr);

public slots:
    /**jsdoc
     * Returns the model/meshes associated with a UUID (entityID, overlayID, or avatarID)
     *
     * @function GraphicsScriptingInterface.getModel
     * @param {UUID} The objectID of the model whose meshes are to be retrieve
     */
    scriptable::ModelProviderPointer getModelProvider(QUuid uuid);
    scriptable::ScriptableModelPointer getModelObject(QUuid uuid);
    bool updateModelObject(QUuid uuid, const scriptable::ScriptableModelPointer model);
    scriptable::ScriptableModelPointer newModelObject(QVector<scriptable::ScriptableMeshPointer> meshes);

#ifdef SCRIPTABLE_MESH_TODO
    scriptable::ScriptableMeshPartPointer exportMeshPart(scriptable::ScriptableMeshPointer mesh, int part=0) {
        return scriptable::make_scriptowned<scriptable::ScriptableMeshPart>(mesh, part);
    }
    bool updateMeshPart(scriptable::ScriptableMeshPointer mesh, scriptable::ScriptableMeshPartPointer part);
#endif

    QString exportModelToOBJ(const scriptable::ScriptableModel& in);

private:
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMeshPointer meshProxy);
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMesh& meshProxy);
    scriptable::MeshPointer getMeshPointer(const scriptable::ScriptableMesh& meshProxy);

};

#endif // hifi_GraphicsScriptingInterface_h
