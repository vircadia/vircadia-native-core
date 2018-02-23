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
     * Returns the meshes associated with a UUID (entityID, overlayID, or avatarID)
     *
     * @function GraphicsScriptingInterface.getMeshes
     * @param {EntityID} entityID The ID of the entity whose meshes are to be retrieve
     */
    QScriptValue getMeshes(QUuid uuid);
    bool updateMeshes(QUuid uuid, const scriptable::ScriptableModelPointer model);
    bool updateMeshes(QUuid uuid, const scriptable::ScriptableMeshPointer mesh, int meshIndex=0, int partIndex=0);

    QString meshToOBJ(const scriptable::ScriptableModel& in);

private:
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMeshPointer meshProxy);
    scriptable::MeshPointer getMeshPointer(scriptable::ScriptableMesh& meshProxy);
    scriptable::MeshPointer getMeshPointer(const scriptable::ScriptableMesh& meshProxy);

};

#endif // hifi_GraphicsScriptingInterface_h
