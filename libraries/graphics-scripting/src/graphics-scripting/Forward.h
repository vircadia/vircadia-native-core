#pragma once

#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QVariant>
#include <QtCore/QUuid>
#include <QPointer>
#include <memory>
#include <unordered_map>

#include <DependencyManager.h>
#include <SpatiallyNestable.h>

#include "graphics/Material.h"
#include "graphics/TextureMap.h"

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

    /**jsdoc
     * @typedef {object} Graphics.Material
     * @property {string} name
     * @property {string} model
     * @property {number|string} opacity
     * @property {number|string} roughness
     * @property {number|string} metallic
     * @property {number|string} scattering
     * @property {boolean|string} unlit
     * @propety {Vec3|string} emissive
     * @propety {Vec3|string} albedo
     * @property {string} emissiveMap
     * @property {string} albedoMap
     * @property {string} opacityMap
     * @property {string} opacityMapMode
     * @property {number|string} opacityCutoff
     * @property {string} metallicMap
     * @property {string} specularMap
     * @property {string} roughnessMap
     * @property {string} glossMap
     * @property {string} normalMap
     * @property {string} bumpMap
     * @property {string} occlusionMap
     * @property {string} lightMap
     * @property {string} scatteringMap
     * @property {Mat4|string} texCoordTransform0
     * @property {Mat4|string} texCoordTransform1
     * @property {string} lightmapParams
     * @property {string} materialParams
     * @property {string} cullFaceMode
     * @property {boolean} defaultFallthrough
     * @property {string} procedural
     */
    class ScriptableMaterial {
    public:
        ScriptableMaterial() {}
        ScriptableMaterial(const graphics::MaterialPointer& material);
        ScriptableMaterial(const ScriptableMaterial& material) { *this = material; }
        ScriptableMaterial& operator=(const ScriptableMaterial& material);

        QString name;
        QString model;
        float opacity;
        float roughness;
        float metallic;
        float scattering;
        bool unlit;
        glm::vec3 emissive;
        glm::vec3 albedo;
        QString emissiveMap;
        QString albedoMap;
        QString opacityMap;
        QString opacityMapMode;
        float opacityCutoff;
        QString metallicMap;
        QString specularMap;
        QString roughnessMap;
        QString glossMap;
        QString normalMap;
        QString bumpMap;
        QString occlusionMap;
        QString lightMap;
        QString scatteringMap;
        std::array<glm::mat4, graphics::Material::NUM_TEXCOORD_TRANSFORMS> texCoordTransforms;
        QString cullFaceMode;
        bool defaultFallthrough;
        std::unordered_map<uint, bool> propertyFallthroughs; // not actually exposed to script

        QString procedural;

        graphics::MaterialKey key { 0 };
    };

    /**jsdoc
     * @typedef {object} Graphics.MaterialLayer
     * @property {Graphics.Material} material - This layer's material.
     * @property {number} priority - The priority of this layer.  If multiple materials are applied to a mesh part, only the highest priority layer is used.
     */
    class ScriptableMaterialLayer {
    public:
        ScriptableMaterialLayer() {}
        ScriptableMaterialLayer(const graphics::MaterialLayer& materialLayer) : material(materialLayer.material), priority(materialLayer.priority) {}
        ScriptableMaterialLayer(const ScriptableMaterialLayer& materialLayer) { *this = materialLayer; }
        ScriptableMaterialLayer& operator=(const ScriptableMaterialLayer& materialLayer);

        ScriptableMaterial material;
        quint16 priority;
    };
    typedef QHash<QString, QVector<scriptable::ScriptableMaterialLayer>> MultiMaterialMap;

    class ScriptableMeshBase : public QObject {
        Q_OBJECT
    public:
        WeakModelProviderPointer provider;
        ScriptableModelBasePointer model;
        WeakMeshPointer weakMesh;
        MeshPointer strongMesh;
        ScriptableMeshBase(WeakModelProviderPointer provider, ScriptableModelBasePointer model, WeakMeshPointer weakMesh, QObject* parent);
        ScriptableMeshBase(WeakMeshPointer weakMesh = WeakMeshPointer(), QObject* parent = nullptr);
        ScriptableMeshBase(const ScriptableMeshBase& other, QObject* parent = nullptr) : QObject(parent) { *this = other; }
        ScriptableMeshBase& operator=(const ScriptableMeshBase& view);
        virtual ~ScriptableMeshBase();
        Q_INVOKABLE const scriptable::MeshPointer getMeshPointer() const { return weakMesh.lock(); }
        Q_INVOKABLE const scriptable::ModelProviderPointer getModelProviderPointer() const { return provider.lock(); }
        Q_INVOKABLE const scriptable::ScriptableModelBasePointer getModelBasePointer() const { return model; }
    };
    
    // abstract container for holding one or more references to mesh pointers
    class ScriptableModelBase : public QObject {
        Q_OBJECT
    public:
        WeakModelProviderPointer provider;
        QUuid objectID; // spatially nestable ID
        QVector<scriptable::ScriptableMeshBase> meshes;
        MultiMaterialMap materialLayers;
        QVector<QString> materialNames;

        ScriptableModelBase(QObject* parent = nullptr) : QObject(parent) {}
        ScriptableModelBase(const ScriptableModelBase& other) : QObject(other.parent()) { *this = other; }
        ScriptableModelBase& operator=(const ScriptableModelBase& other);
        virtual ~ScriptableModelBase();

        void append(const ScriptableMeshBase& mesh);
        void append(scriptable::WeakMeshPointer mesh);
        void appendMaterial(const graphics::MaterialLayer& materialLayer, int shapeID, std::string materialName);
        void appendMaterials(const std::unordered_map<std::string, graphics::MultiMaterial>& materialsToAppend);
        void appendMaterialNames(const std::vector<std::string>& names);
        // TODO: in future containers for these could go here
        // QVariantMap shapes;
        // QVariantMap armature;
    };

    // mixin class for Avatar + Entity Rendering that expose their in-memory graphics::Meshes
    class ModelProvider {
    public:
        NestableType modelProviderType;
        virtual scriptable::ScriptableModelBase getScriptableModel() = 0;
        virtual bool canReplaceModelMeshPart(int meshIndex, int partIndex) { return false; }
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

    class ScriptableModel;
    using ScriptableModelPointer = QPointer<ScriptableModel>;
    class ScriptableMesh;
    using ScriptableMeshPointer = QPointer<ScriptableMesh>;
    class ScriptableMeshPart;
    using ScriptableMeshPartPointer = QPointer<ScriptableMeshPart>;
}
