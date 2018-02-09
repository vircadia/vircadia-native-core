#pragma once

#include <glm/glm.hpp>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtCore/QUuid>
#include <QPointer>
#include <memory>

#include <DependencyManager.h>

namespace graphics {
    class Mesh;
}
namespace gpu {
    class BufferView;
}
class QScriptValue;

namespace scriptable {
    using Mesh = graphics::Mesh;
    using MeshPointer = std::shared_ptr<scriptable::Mesh>;

    class ScriptableModel;
    using ScriptableModelPointer = QPointer<ScriptableModel>;
    class ScriptableMesh;
    using ScriptableMeshPointer = QPointer<ScriptableMesh>;

    // abstract container for holding one or more scriptable meshes
    class ScriptableModel : public QObject {
        Q_OBJECT
    public:
        QUuid objectID;
        QVariantMap metadata;
        QVector<scriptable::MeshPointer> meshes;

        Q_PROPERTY(QVector<scriptable::ScriptableMeshPointer> meshes READ getMeshes)
        Q_PROPERTY(QUuid objectID MEMBER objectID CONSTANT)
        Q_PROPERTY(QVariantMap metadata MEMBER metadata CONSTANT)
        Q_INVOKABLE QString toString() const;

        ScriptableModel(QObject* parent = nullptr) : QObject(parent) {}
        ScriptableModel(const ScriptableModel& other) : objectID(other.objectID), metadata(other.metadata), meshes(other.meshes)  {}
        ScriptableModel& operator=(const ScriptableModel& view) { objectID = view.objectID; metadata = view.metadata; meshes = view.meshes; return *this; }
        ~ScriptableModel() { qDebug() << "~ScriptableModel" << this; }

        void mixin(const ScriptableModel& other) {
            for (const auto& key : other.metadata.keys()) { metadata[key] = other.metadata[key]; }
            for (const auto& mesh : other.meshes) { meshes << mesh; }
        }

        // TODO: in future accessors for these could go here
        // QVariantMap shapes;
        // QVariantMap materials;
        // QVariantMap armature;

        QVector<scriptable::ScriptableMeshPointer> getMeshes();
        const QVector<scriptable::ScriptableMeshPointer> getConstMeshes() const;

        // QScriptEngine-specific wrappers
        Q_INVOKABLE QScriptValue mapAttributeValues(QScriptValue scopeOrCallback, QScriptValue methodOrName);
    };

    // mixin class for Avatar/Entity/Overlay Rendering that expose their in-memory graphics::Meshes
    class ModelProvider {
    public:
        QVariantMap metadata;
        static scriptable::ScriptableModel modelUnavailableError(bool* ok) { if (ok) { *ok = false; } return {}; }
        virtual scriptable::ScriptableModel getScriptableModel(bool* ok = nullptr) = 0;
    };
    using ModelProviderPointer = std::shared_ptr<scriptable::ModelProvider>;

    // mixin class for Application to resolve UUIDs into a corresponding ModelProvider
    class ModelProviderFactory : public Dependency {
    public:
        virtual scriptable::ModelProviderPointer lookupModelProvider(QUuid uuid) = 0;
    };
}

Q_DECLARE_METATYPE(scriptable::MeshPointer)
Q_DECLARE_METATYPE(scriptable::ScriptableModel)
Q_DECLARE_METATYPE(scriptable::ScriptableModelPointer)

