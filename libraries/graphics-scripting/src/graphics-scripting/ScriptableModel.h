#pragma once

#include <glm/glm.hpp>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtCore/QUuid>
#include <memory>

#include <DependencyManager.h>

namespace graphics {
    class Mesh;
}
namespace gpu {
    class BufferView;
}
namespace scriptable {
    using Mesh = graphics::Mesh;
    using MeshPointer = std::shared_ptr<scriptable::Mesh>;

    class ScriptableModel;
    class ScriptableMesh;
    class ScriptableMeshPart;
    using ScriptableModelPointer = std::shared_ptr<scriptable::ScriptableModel>;
    using ScriptableMeshPointer = std::shared_ptr<scriptable::ScriptableMesh>;
    using ScriptableMeshPartPointer = std::shared_ptr<scriptable::ScriptableMeshPart>;
    class ScriptableModel : public QObject, public std::enable_shared_from_this<ScriptableModel> {
        Q_OBJECT
    public:
        Q_PROPERTY(QVector<scriptable::ScriptableMeshPointer> meshes READ getMeshes)

        Q_INVOKABLE QString toString() { return "[ScriptableModel " + objectName()+"]"; }
        ScriptableModel(QObject* parent = nullptr) : QObject(parent) {}
        ScriptableModel(const ScriptableModel& other) : objectID(other.objectID), metadata(other.metadata), meshes(other.meshes)  {}
        ScriptableModel& operator=(const ScriptableModel& view) {
            objectID = view.objectID;
            metadata = view.metadata;
            meshes = view.meshes;
            return *this;
        }
        ~ScriptableModel() { qDebug() << "~ScriptableModel" << this; }
        void mixin(const ScriptableModel& other) {
            for (const auto& key : other.metadata.keys()) {
                metadata[key] = other.metadata[key];
            }
            for(const auto&mesh : other.meshes) {
                meshes << mesh;
            }
        }
        QUuid objectID;
        QVariantMap metadata;
        QVector<scriptable::MeshPointer> meshes;
        // TODO: in future accessors for these could go here
        QVariantMap shapes;
        QVariantMap materials;
        QVariantMap armature;

        QVector<scriptable::ScriptableMeshPointer> getMeshes() const;
    };

    class ModelProvider {
    public:
        QVariantMap metadata;
        static scriptable::ScriptableModel modelUnavailableError(bool* ok) { if (ok) { *ok = false; } return {}; }
        virtual scriptable::ScriptableModel getScriptableModel(bool* ok = nullptr) = 0;
    };
    using ModelProviderPointer = std::shared_ptr<scriptable::ModelProvider>;
    class ModelProviderFactory : public Dependency {
    public:
        virtual scriptable::ModelProviderPointer lookupModelProvider(QUuid uuid) = 0;
    };
        
}

Q_DECLARE_METATYPE(scriptable::MeshPointer)
Q_DECLARE_METATYPE(scriptable::ScriptableModel)
Q_DECLARE_METATYPE(scriptable::ScriptableModelPointer)

