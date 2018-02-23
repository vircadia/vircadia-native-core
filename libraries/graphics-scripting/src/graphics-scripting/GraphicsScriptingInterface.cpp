//
//  GraphicsScriptingInterface.cpp
//  libraries/graphics-scripting/src
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GraphicsScriptingInterface.h"
#include "BufferViewScripting.h"
#include "GraphicsScriptingUtil.h"
#include "OBJWriter.h"
#include "RegisteredMetaTypes.h"
#include "ScriptEngineLogging.h"
#include "ScriptableMesh.h"
#include <GeometryUtil.h>
#include <QUuid>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <graphics/BufferViewHelpers.h>
#include <shared/QtHelpers.h>

GraphicsScriptingInterface::GraphicsScriptingInterface(QObject* parent) : QObject(parent), QScriptable() {
    if (auto scriptEngine = qobject_cast<QScriptEngine*>(parent)) {
        this->registerMetaTypes(scriptEngine);
    }
}

bool GraphicsScriptingInterface::updateModelObject(QUuid uuid, const scriptable::ScriptableModelPointer model) {
    if (auto provider = getModelProvider(uuid)) {
        if (auto base = model->operator scriptable::ScriptableModelBasePointer()) {
#ifdef SCRIPTABLE_MESH_DEBUG
            qDebug() << "replaceScriptableModelMeshPart" << model->toString() << -1 << -1;
#endif
            return provider->replaceScriptableModelMeshPart(base, -1, -1);
        } else {
            qDebug() << "replaceScriptableModelMeshPart -- !base" << model << base << -1 << -1;
        }
    } else {
        qDebug() << "replaceScriptableModelMeshPart -- !provider";
    }

    return false;
}

scriptable::ModelProviderPointer GraphicsScriptingInterface::getModelProvider(QUuid uuid) {
    QString error;
    if (auto appProvider = DependencyManager::get<scriptable::ModelProviderFactory>()) {
        if (auto provider = appProvider->lookupModelProvider(uuid)) {
            return provider;
        } else {
            error = "provider unavailable for " + uuid.toString();
        }
    } else {
        error = "appProvider unavailable";
    }
    if (context()) {
        context()->throwError(error);
    } else {
        qCWarning(graphics_scripting) << "GraphicsScriptingInterface::getModelProvider ERROR" << error;
    }
    return nullptr;
}

scriptable::ScriptableModelPointer GraphicsScriptingInterface::newModelObject(QVector<scriptable::ScriptableMeshPointer> meshes) {
    auto modelWrapper = scriptable::make_scriptowned<scriptable::ScriptableModel>();
    modelWrapper->setObjectName("js::model");
    if (meshes.isEmpty()) {
        if (context()) {
            context()->throwError("expected [meshes] array as first argument");
        }
    } else {
        int i = 0;
        for (const auto& mesh : meshes) {
            if (mesh) {
                modelWrapper->append(*mesh);
            } else if (context()) {
                context()->throwError(QString("invalid mesh at index: %1").arg(i));
            }
            i++;
        }
    }
    return modelWrapper;
}

scriptable::ScriptableModelPointer GraphicsScriptingInterface::getModelObject(QUuid uuid) {
    QString error, providerType = "unknown";
    if (auto provider = getModelProvider(uuid)) {
        providerType = SpatiallyNestable::nestableTypeToString(provider->modelProviderType);
        auto modelObject = provider->getScriptableModel();
        if (modelObject.objectID == uuid) {
            if (modelObject.meshes.size()) {
                auto modelWrapper = scriptable::make_scriptowned<scriptable::ScriptableModel>(modelObject);
                modelWrapper->setObjectName(providerType+"::"+uuid.toString()+"::model");
                return modelWrapper;
            } else {
                error = "no meshes available: " + modelObject.objectID.toString();
            }
        } else {
            error = "objectID mismatch: " + modelObject.objectID.toString();
        }
    } else {
        error = "provider unavailable";
    }
    auto errorMessage = QString("failed to get meshes from %1 provider for uuid %2 (%3)").arg(providerType).arg(uuid.toString()).arg(error);
    qCWarning(graphics_scripting) << "GraphicsScriptingInterface::getModelObject ERROR" << errorMessage;
    if (context()) {
        context()->throwError(errorMessage);
    }
    return nullptr;
}

#ifdef SCRIPTABLE_MESH_TODO
bool GraphicsScriptingInterface::updateMeshPart(scriptable::ScriptableMeshPointer mesh, scriptable::ScriptableMeshPartPointer part) {
    Q_ASSERT(mesh);
    Q_ASSERT(part);
    Q_ASSERT(part->parentMesh);
    auto tmp = exportMeshPart(mesh, part->partIndex);
    if (part->parentMesh == mesh) {
#ifdef SCRIPTABLE_MESH_DEBUG
        qCInfo(graphics_scripting) << "updateMeshPart -- update via clone" << mesh << part;
#endif
        tmp->replaceMeshData(part->cloneMeshPart());
        return false;
    } else {
#ifdef SCRIPTABLE_MESH_DEBUG
        qCInfo(graphics_scripting) << "updateMeshPart -- update via inplace" << mesh << part;
#endif
        tmp->replaceMeshData(part);
        return true;
    }
}
#endif

QString GraphicsScriptingInterface::exportModelToOBJ(const scriptable::ScriptableModel& _in) {
    const auto& in = _in.getConstMeshes();
    if (in.size()) {
        QList<scriptable::MeshPointer> meshes;
        foreach (auto meshProxy, in) {
            if (meshProxy) {
                meshes.append(getMeshPointer(meshProxy));
            }
        }
        if (meshes.size()) {
            return writeOBJToString(meshes);
        }
    }
    if (context()) {
        context()->throwError(QString("null mesh"));
    }
    return QString();
}
void GraphicsScriptingInterface::registerMetaTypes(QScriptEngine* engine) {
    scriptable::registerMetaTypes(engine);
}

MeshPointer GraphicsScriptingInterface::getMeshPointer(const scriptable::ScriptableMesh& meshProxy) {
    return meshProxy.getMeshPointer();
}
MeshPointer GraphicsScriptingInterface::getMeshPointer(scriptable::ScriptableMesh& meshProxy) {
    return getMeshPointer(&meshProxy);
}
MeshPointer GraphicsScriptingInterface::getMeshPointer(scriptable::ScriptableMeshPointer meshProxy) {
    MeshPointer result;
    if (!meshProxy) {
        if (context()){
            context()->throwError("expected meshProxy as first parameter");
        } else {
            qCDebug(graphics_scripting) << "expected meshProxy as first parameter";
        }
        return result;
    }
    auto mesh = meshProxy->getMeshPointer();
    if (!mesh) {
        if (context()) {
            context()->throwError("expected valid meshProxy as first parameter");
        } else {
            qCDebug(graphics_scripting) << "expected valid meshProxy as first parameter";
        }
        return result;
    }
    return mesh;
}

#include "GraphicsScriptingInterface.moc"
