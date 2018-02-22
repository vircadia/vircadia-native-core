#pragma once

#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QVariant>
#include <QtCore/QUuid>
#include <QPointer>
#include <memory>

#include <DependencyManager.h>
#include <SpatiallyNestable.h>
namespace graphics {
    class Mesh;
}
class Model;
using ModelPointer = std::shared_ptr<Model>;
namespace gpu {
    class BufferView;
}
class QScriptEngine;

namespace scriptable {
    using Mesh = graphics::Mesh;
    using MeshPointer = std::shared_ptr<scriptable::Mesh>;
    using WeakMeshPointer = std::weak_ptr<scriptable::Mesh>;

    class ScriptableModelBase;
    using ScriptableModelBasePointer = QPointer<ScriptableModelBase>;

    class ModelProvider;
    using ModelProviderPointer = std::shared_ptr<scriptable::ModelProvider>;
    using WeakModelProviderPointer = std::weak_ptr<scriptable::ModelProvider>;

    class ScriptableMeshBase : public QObject {
        Q_OBJECT
    public:
        WeakModelProviderPointer provider;
        ScriptableModelBasePointer model;
        WeakMeshPointer mesh;
        MeshPointer ownedMesh;
        QVariantMap metadata;
        ScriptableMeshBase(WeakModelProviderPointer provider, ScriptableModelBasePointer model, WeakMeshPointer mesh, const QVariantMap& metadata);
        ScriptableMeshBase(WeakMeshPointer mesh = WeakMeshPointer());
        ScriptableMeshBase(MeshPointer mesh, const QVariantMap& metadata);
        ScriptableMeshBase(const ScriptableMeshBase& other) : QObject() { *this = other; }
        ScriptableMeshBase& operator=(const ScriptableMeshBase& view);
        virtual ~ScriptableMeshBase();
        Q_INVOKABLE const scriptable::MeshPointer getMeshPointer() const { return mesh.lock(); }
        Q_INVOKABLE const scriptable::ModelProviderPointer getModelProviderPointer() const { return provider.lock(); }
        Q_INVOKABLE const scriptable::ScriptableModelBasePointer getModelBasePointer() const { return model; }
    };
    
    // abstract container for holding one or more references to mesh pointers
    class ScriptableModelBase : public QObject {
        Q_OBJECT
    public:
        WeakModelProviderPointer provider;
        QUuid objectID; // spatially nestable ID
        QVariantMap metadata;
        QVector<scriptable::ScriptableMeshBase> meshes;

        ScriptableModelBase(QObject* parent = nullptr) : QObject(parent) {}
        ScriptableModelBase(const ScriptableModelBase& other) : QObject() { *this = other; }
        ScriptableModelBase& operator=(const ScriptableModelBase& other);
        virtual ~ScriptableModelBase();

        void mixin(const QVariantMap& other);
        void append(const ScriptableModelBase& other, const QVariantMap& modelMetadata = QVariantMap());
        void append(scriptable::WeakMeshPointer mesh, const QVariantMap& metadata = QVariantMap());
        void append(const ScriptableMeshBase& mesh, const QVariantMap& metadata = QVariantMap());
        // TODO: in future containers for these could go here
        // QVariantMap shapes;
        // QVariantMap materials;
        // QVariantMap armature;
    };

    // mixin class for Avatar/Entity/Overlay Rendering that expose their in-memory graphics::Meshes
    class ModelProvider {
    public:
        NestableType modelProviderType;
        static scriptable::ScriptableModelBase modelUnavailableError(bool* ok) { if (ok) { *ok = false; } return {}; }
        virtual scriptable::ScriptableModelBase getScriptableModel(bool* ok = nullptr) = 0;

        virtual bool replaceScriptableModelMeshPart(scriptable::ScriptableModelBasePointer model, int meshIndex, int partIndex) { return false; }
    };

    // mixin class for resolving UUIDs into a corresponding ModelProvider
    class ModelProviderFactory : public QObject, public Dependency {
        Q_OBJECT
    public:
        virtual scriptable::ModelProviderPointer lookupModelProvider(const QUuid& uuid) = 0;
    signals:
        void modelAddedToScene(const QUuid& objectID, NestableType nestableType, const ModelPointer& sender);
        void modelRemovedFromScene(const QUuid& objectID, NestableType nestableType, const ModelPointer& sender);
    };

    using uint32 = quint32;
    class ScriptableModel;
    using ScriptableModelPointer = QPointer<ScriptableModel>;
    class ScriptableMesh;
    using ScriptableMeshPointer = QPointer<ScriptableMesh>;
    class ScriptableMeshPart;
    using ScriptableMeshPartPointer = QPointer<ScriptableMeshPart>;
    bool registerMetaTypes(QScriptEngine* engine);
}

Q_DECLARE_METATYPE(NestableType)
