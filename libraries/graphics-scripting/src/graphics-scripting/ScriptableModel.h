//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "Forward.h"
#include "GraphicsScriptingUtil.h"

class QScriptValue;

namespace scriptable {

    using ScriptableMeshes = QVector<scriptable::ScriptableMeshPointer>;
    class ScriptableModel : public ScriptableModelBase {
        Q_OBJECT
        Q_PROPERTY(QUuid objectID MEMBER objectID CONSTANT)
        Q_PROPERTY(glm::uint32 numMeshes READ getNumMeshes)
        Q_PROPERTY(ScriptableMeshes meshes READ getMeshes)
        Q_PROPERTY(scriptable::MultiMaterialMap materials READ getMaterials)
        Q_PROPERTY(QVector<QString> materialNames READ getMaterialNames)

    public:
        ScriptableModel(QObject* parent = nullptr) : ScriptableModelBase(parent) {}
        ScriptableModel(const ScriptableModel& other) : ScriptableModelBase(other) {}
        ScriptableModel(const ScriptableModelBase& other) : ScriptableModelBase(other) {}
        ScriptableModel& operator=(const ScriptableModelBase& view) {  ScriptableModelBase::operator=(view); return *this; }

        operator scriptable::ScriptableModelBasePointer() {
            return QPointer<scriptable::ScriptableModelBase>(qobject_cast<scriptable::ScriptableModelBase*>(this));
        }
        ScriptableMeshes getMeshes();
        const ScriptableMeshes getConstMeshes() const;

        scriptable::MultiMaterialMap getMaterials() { return materials; }
        QVector<QString> getMaterialNames() { return materialNames; }

    public slots:
        scriptable::ScriptableModelPointer cloneModel(const QVariantMap& options = QVariantMap());
        QString toString() const;

        // QScriptEngine-specific wrappers
        //glm::uint32 forEachMeshVertexAttribute(QScriptValue callback);
    protected:
        glm::uint32 getNumMeshes() { return meshes.size(); }

    };

}

Q_DECLARE_METATYPE(scriptable::MeshPointer)
Q_DECLARE_METATYPE(scriptable::WeakMeshPointer)
Q_DECLARE_METATYPE(scriptable::ScriptableModelPointer)
Q_DECLARE_METATYPE(scriptable::ScriptableModelBase)
Q_DECLARE_METATYPE(scriptable::ScriptableModelBasePointer)
Q_DECLARE_METATYPE(scriptable::ScriptableMaterial)
Q_DECLARE_METATYPE(scriptable::MultiMaterialMap)
