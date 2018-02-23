#pragma once

#include "Forward.h"
#include "GraphicsScriptingUtil.h"

class QScriptValue;
namespace scriptable {
    class ScriptableModel : public ScriptableModelBase {
        Q_OBJECT
    public:
        Q_PROPERTY(QUuid objectID MEMBER objectID CONSTANT)
        Q_PROPERTY(QVariantMap metadata MEMBER metadata CONSTANT)
        Q_PROPERTY(uint32 numMeshes READ getNumMeshes)
        Q_PROPERTY(QVector<scriptable::ScriptableMeshPointer> meshes READ getMeshes)

        ScriptableModel(QObject* parent = nullptr) : ScriptableModelBase(parent) {}
        ScriptableModel(const ScriptableModel& other) : ScriptableModelBase(other) {}
        ScriptableModel(const ScriptableModelBase& other) : ScriptableModelBase(other) {}
        ScriptableModel& operator=(const ScriptableModelBase& view) {  ScriptableModelBase::operator=(view); return *this; }

        Q_INVOKABLE scriptable::ScriptableModelPointer cloneModel(const QVariantMap& options = QVariantMap());
        // TODO: in future accessors for these could go here
        // QVariantMap shapes;
        // QVariantMap materials;
        // QVariantMap armature;

        QVector<scriptable::ScriptableMeshPointer> getMeshes();
        const QVector<scriptable::ScriptableMeshPointer> getConstMeshes() const;
        operator scriptable::ScriptableModelBasePointer() {
            QPointer<scriptable::ScriptableModelBase> p;
            p = qobject_cast<scriptable::ScriptableModelBase*>(this);
            return p;
        }
        

        // QScriptEngine-specific wrappers
        Q_INVOKABLE uint32 mapAttributeValues(QScriptValue callback);
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE uint32 getNumMeshes() { return meshes.size(); }
    };

}

Q_DECLARE_METATYPE(scriptable::MeshPointer)
Q_DECLARE_METATYPE(scriptable::WeakMeshPointer)
Q_DECLARE_METATYPE(scriptable::ScriptableModelPointer)
Q_DECLARE_METATYPE(scriptable::ScriptableModelBase)
Q_DECLARE_METATYPE(scriptable::ScriptableModelBasePointer)
