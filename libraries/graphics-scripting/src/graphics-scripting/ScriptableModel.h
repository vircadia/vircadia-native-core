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

    /**jsdoc
     * @typedef {object} Graphics.Model
     * @property {Uuid} objectID - UUID of corresponding inworld object (if model is associated)
     * @property {number} numMeshes - The number of submeshes contained in the model.
     * @property {Graphics.Mesh[]} meshes - Array of submesh references.
     * @property {Object.<string,Graphics.MaterialLayer[]>} materialLayers - Map of materials layer lists.  You can look up a material layer list by mesh part number or by material name.
     * @property {string[]} materialNames - Array of all the material names used by the mesh parts of this model, in order (e.g. materialNames[0] is the name of the first mesh part's material).
     */

    class ScriptableModel : public ScriptableModelBase {
        Q_OBJECT
        Q_PROPERTY(QUuid objectID MEMBER objectID CONSTANT)
        Q_PROPERTY(glm::uint32 numMeshes READ getNumMeshes)
        Q_PROPERTY(ScriptableMeshes meshes READ getMeshes)
        Q_PROPERTY(scriptable::MultiMaterialMap materialLayers READ getMaterialLayers)
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

        scriptable::MultiMaterialMap getMaterialLayers() { return materialLayers; }
        QVector<QString> getMaterialNames() { return materialNames; }

    public slots:
        scriptable::ScriptableModelPointer cloneModel(const QVariantMap& options = QVariantMap());
        QString toString() const;

    protected:
        glm::uint32 getNumMeshes() { return meshes.size(); }

    };

}

Q_DECLARE_METATYPE(scriptable::ScriptableModelPointer)
Q_DECLARE_METATYPE(QVector<scriptable::ScriptableModelPointer>)
Q_DECLARE_METATYPE(scriptable::ScriptableMaterial)
Q_DECLARE_METATYPE(scriptable::ScriptableMaterialLayer)
Q_DECLARE_METATYPE(scriptable::MultiMaterialMap)
