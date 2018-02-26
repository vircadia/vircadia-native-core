//
//  ScriptableModel.cpp
//  libraries/graphics-scripting
//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GraphicsScriptingUtil.h"
#include "ScriptableModel.h"
#include "ScriptableMesh.h"

#include <QtScript/QScriptEngine>

// #define SCRIPTABLE_MESH_DEBUG 1

scriptable::ScriptableModelBase& scriptable::ScriptableModelBase::operator=(const scriptable::ScriptableModelBase& other) {
    provider = other.provider;
    objectID = other.objectID;
    for (const auto& mesh : other.meshes) {
        append(mesh);
    }
    return *this;
}

scriptable::ScriptableModelBase::~ScriptableModelBase() {
#ifdef SCRIPTABLE_MESH_DEBUG
    qCDebug(graphics_scripting) << "~ScriptableModelBase" << this;
#endif
    // makes cleanup order more deterministic to help with debugging
    for (auto& m : meshes) {
        m.strongMesh.reset();
    }
    meshes.clear();
}

void scriptable::ScriptableModelBase::append(scriptable::WeakMeshPointer mesh) {
    meshes << ScriptableMeshBase{ provider, this, mesh, this /*parent*/ };
}

void scriptable::ScriptableModelBase::append(const ScriptableMeshBase& mesh) {
    if (mesh.provider.lock().get() != provider.lock().get()) {
        qCDebug(graphics_scripting) << "warning: appending mesh from different provider..." << mesh.provider.lock().get() << " != " << provider.lock().get();
    }
    meshes << mesh;
}

QString scriptable::ScriptableModel::toString() const {
    return QString("[ScriptableModel%1%2 numMeshes=%3]")
        .arg(objectID.isNull() ? "" : " objectID="+objectID.toString())
        .arg(objectName().isEmpty() ? "" : " name=" +objectName())
        .arg(meshes.size());
}

scriptable::ScriptableModelPointer scriptable::ScriptableModel::cloneModel(const QVariantMap& options) {
    scriptable::ScriptableModelPointer clone = scriptable::ScriptableModelPointer(new scriptable::ScriptableModel(*this));
    clone->meshes.clear();
    for (const auto &mesh : getConstMeshes()) {
        auto cloned = mesh->cloneMesh();
        if (auto tmp = qobject_cast<scriptable::ScriptableMeshBase*>(cloned)) {
            clone->meshes << *tmp;
            tmp->deleteLater(); // schedule our copy for cleanup
        } else {
            qCDebug(graphics_scripting) << "error cloning mesh" << cloned;
        }
    }
    return clone;
}


const scriptable::ScriptableMeshes scriptable::ScriptableModel::getConstMeshes() const {
    scriptable::ScriptableMeshes out;
    for (const auto& mesh : meshes) {
        const scriptable::ScriptableMesh* m = qobject_cast<const scriptable::ScriptableMesh*>(&mesh);
        if (!m) {
            m = scriptable::make_scriptowned<scriptable::ScriptableMesh>(mesh);
        } else {
            qCDebug(graphics_scripting) << "reusing scriptable mesh" << m;
        }
        const scriptable::ScriptableMeshPointer mp = scriptable::ScriptableMeshPointer(const_cast<scriptable::ScriptableMesh*>(m));
        out << mp;
    }
    return out;
}

scriptable::ScriptableMeshes scriptable::ScriptableModel::getMeshes() {
    scriptable::ScriptableMeshes out;
    for (auto& mesh : meshes) {
        scriptable::ScriptableMesh* m = qobject_cast<scriptable::ScriptableMesh*>(&mesh);
        if (!m) {
            m = scriptable::make_scriptowned<scriptable::ScriptableMesh>(mesh);
        } else {
            qCDebug(graphics_scripting) << "reusing scriptable mesh" << m;
        }
        scriptable::ScriptableMeshPointer mp = scriptable::ScriptableMeshPointer(m);
        out << mp;
    }
    return out;
}

#if 0
glm::uint32 scriptable::ScriptableModel::forEachVertexAttribute(QScriptValue callback) {
    glm::uint32 result = 0;
    scriptable::ScriptableMeshes in = getMeshes();
    if (in.size()) {
        foreach (scriptable::ScriptableMeshPointer meshProxy, in) {
            result += meshProxy->mapAttributeValues(callback);
        }
    }
    return result;
}
#endif

#include "ScriptableModel.moc"
