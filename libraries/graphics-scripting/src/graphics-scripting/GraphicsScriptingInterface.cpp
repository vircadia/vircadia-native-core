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
#include "BaseScriptEngine.h"
#include "BufferViewScripting.h"
#include "DebugNames.h"
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

#include "GraphicsScriptingInterface.moc"

GraphicsScriptingInterface::GraphicsScriptingInterface(QObject* parent) : QObject(parent), QScriptable() {
    if (auto scriptEngine = qobject_cast<QScriptEngine*>(parent)) {
        this->registerMetaTypes(scriptEngine);
    }
}

bool GraphicsScriptingInterface::updateMeshes(QUuid uuid, const scriptable::ScriptableMeshPointer mesh, int meshIndex, int partIndex) {
    auto model = scriptable::make_qtowned<scriptable::ScriptableModel>();
    if (mesh) {
        model->append(*mesh);
    }
    return updateMeshes(uuid, model.get());
}

bool GraphicsScriptingInterface::updateMeshes(QUuid uuid, const scriptable::ScriptableModelPointer model) {
    auto appProvider = DependencyManager::get<scriptable::ModelProviderFactory>();
    scriptable::ModelProviderPointer provider = appProvider ? appProvider->lookupModelProvider(uuid) : nullptr;
    QString providerType = provider ? SpatiallyNestable::nestableTypeToString(provider->modelProviderType) : QString();
    if (providerType.isEmpty()) {
        providerType = "unknown";
    }
    bool success = false;
    if (provider) {
        auto scriptableMeshes = provider->getScriptableModel(&success);
        if (success) {
            const scriptable::ScriptableModelBasePointer base = model->operator scriptable::ScriptableModelBasePointer();
            if (base) {
                success = provider->replaceScriptableModelMeshPart(base, -1, -1);
            }
        }
    }
    return success;
}

QScriptValue GraphicsScriptingInterface::getMeshes(QUuid uuid) {
    scriptable::ScriptableModel* meshes{ nullptr };
    bool success = false;
    QString error;

    auto appProvider = DependencyManager::get<scriptable::ModelProviderFactory>();
    qCDebug(graphics_scripting) << "appProvider" << appProvider.data();
    scriptable::ModelProviderPointer provider = appProvider ? appProvider->lookupModelProvider(uuid) : nullptr;
    QString providerType = provider ? SpatiallyNestable::nestableTypeToString(provider->modelProviderType) : QString();
    if (providerType.isEmpty()) {
        providerType = "unknown";
    }
    if (provider) {
        auto scriptableMeshes = provider->getScriptableModel(&success);
        if (success) {
            meshes = scriptable::make_scriptowned<scriptable::ScriptableModel>(scriptableMeshes);
            if (meshes->objectName().isEmpty()) {
                meshes->setObjectName(providerType+"::meshes");
            }
            if (meshes->objectID.isNull()) {
                meshes->objectID = uuid.toString();
            }
            meshes->metadata["provider"] = SpatiallyNestable::nestableTypeToString(provider->modelProviderType);
        }
    }
    if (!success) {
        error = QString("failed to get meshes from %1 provider for uuid %2").arg(providerType).arg(uuid.toString());
    }

    QPointer<BaseScriptEngine> scriptEngine = dynamic_cast<BaseScriptEngine*>(engine());    
    QScriptValue result = error.isEmpty() ? scriptEngine->toScriptValue(meshes) : scriptEngine->makeError(error);
    if (result.isError()) {
        qCWarning(graphics_scripting) << "GraphicsScriptingInterface::getMeshes ERROR" << result.toString();
        if (context()) {
            context()->throwValue(error);
        } else {
            qCWarning(graphics_scripting) << "GraphicsScriptingInterface::getMeshes ERROR" << result.toString();
        }
        return QScriptValue::NullValue;
    }
    return scriptEngine->toScriptValue(meshes);
}

QString GraphicsScriptingInterface::meshToOBJ(const scriptable::ScriptableModel& _in) {
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
