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
#include "GraphicsScriptingUtil.h"
#include "OBJWriter.h"
#include "RegisteredMetaTypes.h"
#include "ScriptEngineLogging.h"
#include "ScriptableMesh.h"
#include "ScriptableMeshPart.h"
#include <GeometryUtil.h>
#include <QUuid>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <graphics/BufferViewHelpers.h>
#include <graphics/GpuHelpers.h>
#include <shared/QtHelpers.h>
#include <SpatiallyNestable.h>

GraphicsScriptingInterface::GraphicsScriptingInterface(QObject* parent) : QObject(parent), QScriptable() {
}

void GraphicsScriptingInterface::jsThrowError(const QString& error) {
    if (context()) {
        context()->throwError(error);
    } else {
        qCWarning(graphics_scripting) << "GraphicsScriptingInterface::jsThrowError (without valid JS context):" << error;
    }
}

bool GraphicsScriptingInterface::canUpdateModel(QUuid uuid, int meshIndex, int partNumber) {
    auto provider = getModelProvider(uuid);
    return provider && provider->canReplaceModelMeshPart(meshIndex, partNumber);
}

bool GraphicsScriptingInterface::updateModel(QUuid uuid, const scriptable::ScriptableModelPointer& model) {
    if (!model) {
        jsThrowError("null model argument");
    }

    auto base = model->operator scriptable::ScriptableModelBasePointer();
    if (!base) {
        jsThrowError("could not get base model pointer");
        return false;
    }

    auto provider = getModelProvider(uuid);
    if (!provider) {
        jsThrowError("provider unavailable");
        return false;
    }

    if (!provider->canReplaceModelMeshPart(-1, -1)) {
        jsThrowError("provider does not support updating mesh parts");
        return false;
    }

#ifdef SCRIPTABLE_MESH_DEBUG
    qDebug() << "replaceScriptableModelMeshPart" << model->toString() << -1 << -1;
#endif
    return provider->replaceScriptableModelMeshPart(base, -1, -1);
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
    jsThrowError(error);
    return nullptr;
}

scriptable::ScriptableModelPointer GraphicsScriptingInterface::newModel(const scriptable::ScriptableMeshes& meshes) {
    auto modelWrapper = scriptable::make_scriptowned<scriptable::ScriptableModel>();
    modelWrapper->setObjectName("js::model");
    if (meshes.isEmpty()) {
        jsThrowError("expected [meshes] array as first argument");
    } else {
        int i = 0;
        for (const auto& mesh : meshes) {
#ifdef SCRIPTABLE_MESH_DEBUG
            qDebug() << "newModel" << i << meshes.size() << mesh;
#endif
            if (mesh) {
                modelWrapper->append(*mesh);
            } else {
                jsThrowError(QString("invalid mesh at index: %1").arg(i));
                break;
            }
            i++;
        }
    }
    return modelWrapper;
}

scriptable::ScriptableModelPointer GraphicsScriptingInterface::getModel(QUuid uuid) {
    QString error;
    bool success;
    QString providerType = "unknown";
    if (auto nestable = DependencyManager::get<SpatialParentFinder>()->find(uuid, success).lock()) {
        providerType = SpatiallyNestable::nestableTypeToString(nestable->getNestableType());
        if (auto provider = getModelProvider(uuid)) {
            auto modelObject = provider->getScriptableModel();
            const bool found = !modelObject.objectID.isNull();
            if (found && uuid == AVATAR_SELF_ID) {
                // special case override so that scripts can rely on matching intput/output UUIDs
                modelObject.objectID = AVATAR_SELF_ID;
            }
            if (modelObject.objectID == uuid) {
                if (modelObject.meshes.size()) {
                    auto modelWrapper = scriptable::make_scriptowned<scriptable::ScriptableModel>(modelObject);
                    modelWrapper->setObjectName(providerType+"::"+uuid.toString()+"::model");
                    return modelWrapper;
                } else {
                    error = "no meshes available: " + modelObject.objectID.toString();
                }
            } else {
                error = QString("objectID mismatch: %1 (result contained %2 meshes)").arg(modelObject.objectID.toString()).arg(modelObject.meshes.size());
            }
        } else {
            error = "model provider unavailable";
        }
    } else {
        error = "model object not found";
    }
    jsThrowError(QString("failed to get meshes from %1 provider for uuid %2 (%3)").arg(providerType).arg(uuid.toString()).arg(error));
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

scriptable::ScriptableMeshPointer GraphicsScriptingInterface::newMesh(const QVariantMap& ifsMeshData) {
    // TODO: this is bare-bones way for now to improvise a new mesh from the scripting side
    //  in the future we want to support a formal C++ structure data type here instead

    /**jsdoc
     * @typedef {object} Graphics.IFSData
     * @property {string} [name=""] - mesh name (useful for debugging / debug prints).
     * @property {string} [topology=""]
     * @property {number[]} indices - vertex indices to use for the mesh faces.
     * @property {Vec3[]} vertices - vertex positions (model space)
     * @property {Vec3[]} [normals=[]] - vertex normals (normalized)
     * @property {Vec3[]} [colors=[]] - vertex colors (normalized)
     * @property {Vec2[]} [texCoords0=[]] - vertex texture coordinates (normalized)
     */
    QString meshName = ifsMeshData.value("name").toString();
    QString topologyName = ifsMeshData.value("topology").toString();
    QVector<glm::uint32> indices = buffer_helpers::variantToVector<glm::uint32>(ifsMeshData.value("indices"));
    QVector<glm::vec3> vertices = buffer_helpers::variantToVector<glm::vec3>(ifsMeshData.value("positions"));
    QVector<glm::vec3> normals = buffer_helpers::variantToVector<glm::vec3>(ifsMeshData.value("normals"));
    QVector<glm::vec3> colors = buffer_helpers::variantToVector<glm::vec3>(ifsMeshData.value("colors"));
    QVector<glm::vec2> texCoords0 = buffer_helpers::variantToVector<glm::vec2>(ifsMeshData.value("texCoords0"));

    const auto numVertices = vertices.size();
    const auto numIndices = indices.size();
    const auto topology = graphics::TOPOLOGIES.key(topologyName);

    // TODO: support additional topologies (POINTS and LINES ought to "just work" --
    //   if MeshPartPayload::drawCall is updated to actually check the Mesh::Part::_topology value
    //   (TRIANGLE_STRIP, TRIANGLE_FAN, QUADS, QUAD_STRIP may need additional conversion code though)
    static const QStringList acceptableTopologies{ "triangles" };

    // sanity checks
    QString error;
    if (!topologyName.isEmpty() && !acceptableTopologies.contains(topologyName)) {
        error = QString("expected .topology to be %1").arg(acceptableTopologies.join(" | "));
    } else if (!numIndices) {
        error = QString("expected non-empty [uint32,...] array for .indices (got type=%1)").arg(ifsMeshData.value("indices").typeName());
    } else if (numIndices % 3 != 0) {
        error = QString("expected 'triangle faces' for .indices (ie: length to be divisible by 3) length=%1").arg(numIndices);
    } else if (!numVertices) {
        error = "expected non-empty [glm::vec3(),...] array for .positions";
    } else {
        const gpu::uint32 maxVertexIndex = numVertices;
        int i = 0;
        for (const auto& ind : indices) {
            if (ind >= maxVertexIndex) {
                error = QString("index out of .indices[%1] index=%2 >= maxVertexIndex=%3").arg(i).arg(ind).arg(maxVertexIndex);
                break;
            }
            i++;
        }
    }
    if (!error.isEmpty()) {
        jsThrowError(error);
        return nullptr;
    }

    if (ifsMeshData.contains("normals") && normals.size() < numVertices) {
        qCInfo(graphics_scripting) << "newMesh -- expanding .normals to #" << numVertices;
        normals.resize(numVertices);
    }
    if (ifsMeshData.contains("colors") && colors.size() < numVertices) {
        qCInfo(graphics_scripting) << "newMesh -- expanding .colors to #" << numVertices;
        colors.resize(numVertices);
    }
    if (ifsMeshData.contains("texCoords0") && texCoords0.size() < numVertices) {
        qCInfo(graphics_scripting) << "newMesh -- expanding .texCoords0 to #" << numVertices;
        texCoords0.resize(numVertices);
    }
    if (ifsMeshData.contains("texCoords1")) {
        qCWarning(graphics_scripting) << "newMesh - texCoords1 not yet supported; ignoring";
    }

    graphics::MeshPointer mesh(new graphics::Mesh());
    mesh->modelName = "graphics::newMesh";
    mesh->displayName = meshName.toStdString();

    // TODO: newFromVector does inbound type conversion, but not compression or packing
    //  (later we should autodetect if fitting into gpu::INDEX_UINT16 and reduce / pack normals etc.)
    mesh->setIndexBuffer(buffer_helpers::newFromVector(indices, gpu::Format::INDEX_INT32));
    mesh->setVertexBuffer(buffer_helpers::newFromVector(vertices, gpu::Format::VEC3F_XYZ));
    if (normals.size()) {
        mesh->addAttribute(gpu::Stream::NORMAL, buffer_helpers::newFromVector(normals, gpu::Format::VEC3F_XYZ));
    }
    if (colors.size()) {
        mesh->addAttribute(gpu::Stream::COLOR, buffer_helpers::newFromVector(colors, gpu::Format::VEC3F_XYZ));
    }
    if (texCoords0.size()) {
        mesh->addAttribute(gpu::Stream::TEXCOORD0, buffer_helpers::newFromVector(texCoords0, gpu::Format::VEC2F_UV));
    }
    QVector<graphics::Mesh::Part> parts = {{ 0, indices.size(), 0, topology }};
    mesh->setPartBuffer(buffer_helpers::newFromVector(parts, gpu::Element::PART_DRAWCALL));
    return scriptable::make_scriptowned<scriptable::ScriptableMesh>(mesh, nullptr);
}

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
    jsThrowError("null mesh");
    return QString();
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
        jsThrowError("expected meshProxy as first parameter");
        return result;
    }
    auto mesh = meshProxy->getMeshPointer();
    if (!mesh) {
        jsThrowError("expected valid meshProxy as first parameter");
        return result;
    }
    return mesh;
}

namespace {
    QVector<int> metaTypeIds{
        qRegisterMetaType<glm::uint32>("uint32"),
        qRegisterMetaType<glm::uint32>("glm::uint32"),
        qRegisterMetaType<QVector<glm::uint32>>(),
        qRegisterMetaType<QVector<glm::uint32>>("QVector<uint32>"),
        qRegisterMetaType<scriptable::ScriptableMeshes>(),
        qRegisterMetaType<scriptable::ScriptableMeshes>("ScriptableMeshes"),
        qRegisterMetaType<scriptable::ScriptableMeshes>("scriptable::ScriptableMeshes"),
        qRegisterMetaType<QVector<scriptable::ScriptableMeshPointer>>("QVector<scriptable::ScriptableMeshPointer>"),
        qRegisterMetaType<scriptable::ScriptableMeshPointer>(),
        qRegisterMetaType<scriptable::ScriptableModelPointer>(),
        qRegisterMetaType<scriptable::ScriptableMeshPartPointer>(),
        qRegisterMetaType<scriptable::ScriptableMaterial>(),
        qRegisterMetaType<scriptable::ScriptableMaterialLayer>(),
        qRegisterMetaType<QVector<scriptable::ScriptableMaterialLayer>>(),
        qRegisterMetaType<scriptable::MultiMaterialMap>(),
        qRegisterMetaType<graphics::Mesh::Topology>(),
    };
}

namespace scriptable {
    template <typename T> int registerQPointerMetaType(QScriptEngine* engine) {
        qScriptRegisterSequenceMetaType<QVector<QPointer<T>>>(engine);
        return qScriptRegisterMetaType<QPointer<T>>(
            engine,
            [](QScriptEngine* engine, const QPointer<T>& object) -> QScriptValue {
                if (!object) {
                    return QScriptValue::NullValue;
                }
                return engine->newQObject(object, QScriptEngine::QtOwnership, QScriptEngine::ExcludeDeleteLater | QScriptEngine::AutoCreateDynamicProperties);
            },
            [](const QScriptValue& value, QPointer<T>& out) {
                auto obj = value.toQObject();
#ifdef SCRIPTABLE_MESH_DEBUG
                qCInfo(graphics_scripting) << "qpointer_qobject_cast" << obj << value.toString();
#endif
                if (auto tmp = qobject_cast<T*>(obj)) {
                    out = QPointer<T>(tmp);
                    return;
                }
#if 0
                if (auto tmp = static_cast<T*>(obj)) {
#ifdef SCRIPTABLE_MESH_DEBUG
                    qCInfo(graphics_scripting) << "qpointer_qobject_cast -- via static_cast" << obj << tmp << value.toString();
#endif
                    out = QPointer<T>(tmp);
                    return;
                }
#endif
                out = nullptr;
            }
        );
    }

    QScriptValue qVectorScriptableMaterialLayerToScriptValue(QScriptEngine* engine, const QVector<scriptable::ScriptableMaterialLayer>& vector) {
        return qScriptValueFromSequence(engine, vector);
    }

    void qVectorScriptableMaterialLayerFromScriptValue(const QScriptValue& array, QVector<scriptable::ScriptableMaterialLayer>& result) {
        qScriptValueToSequence(array, result);
    }

    QScriptValue scriptableMaterialToScriptValue(QScriptEngine* engine, const scriptable::ScriptableMaterial &material) {
        QScriptValue obj = engine->newObject();
        obj.setProperty("name", material.name);
        obj.setProperty("model", material.model);
        obj.setProperty("opacity", material.opacity);
        obj.setProperty("roughness", material.roughness);
        obj.setProperty("metallic", material.metallic);
        obj.setProperty("scattering", material.scattering);
        obj.setProperty("unlit", material.unlit);
        obj.setProperty("emissive", vec3ColorToScriptValue(engine, material.emissive));
        obj.setProperty("albedo", vec3ColorToScriptValue(engine, material.albedo));
        obj.setProperty("emissiveMap", material.emissiveMap);
        obj.setProperty("albedoMap", material.albedoMap);
        obj.setProperty("opacityMap", material.opacityMap);
        obj.setProperty("metallicMap", material.metallicMap);
        obj.setProperty("specularMap", material.specularMap);
        obj.setProperty("roughnessMap", material.roughnessMap);
        obj.setProperty("glossMap", material.glossMap);
        obj.setProperty("normalMap", material.normalMap);
        obj.setProperty("bumpMap", material.bumpMap);
        obj.setProperty("occlusionMap", material.occlusionMap);
        obj.setProperty("lightmapMap", material.lightmapMap);
        obj.setProperty("scatteringMap", material.scatteringMap);
        return obj;
    }

    void scriptableMaterialFromScriptValue(const QScriptValue &object, scriptable::ScriptableMaterial& material) {
        // No need to convert from QScriptValue to ScriptableMaterial
    }

    QScriptValue scriptableMaterialLayerToScriptValue(QScriptEngine* engine, const scriptable::ScriptableMaterialLayer &materialLayer) {
        QScriptValue obj = engine->newObject();
        obj.setProperty("material", scriptableMaterialToScriptValue(engine, materialLayer.material));
        obj.setProperty("priority", materialLayer.priority);
        return obj;
    }

    void scriptableMaterialLayerFromScriptValue(const QScriptValue &object, scriptable::ScriptableMaterialLayer& materialLayer) {
        // No need to convert from QScriptValue to ScriptableMaterialLayer
    }

    QScriptValue multiMaterialMapToScriptValue(QScriptEngine* engine, const scriptable::MultiMaterialMap& map) {
        QScriptValue obj = engine->newObject();
        for (auto key : map.keys()) {
            obj.setProperty(key, qVectorScriptableMaterialLayerToScriptValue(engine, map[key]));
        }
        return obj;
    }

    void multiMaterialMapFromScriptValue(const QScriptValue& map, scriptable::MultiMaterialMap& result) {
        // No need to convert from QScriptValue to MultiMaterialMap
    }

    template <typename T> int registerDebugEnum(QScriptEngine* engine, const DebugEnums<T>& debugEnums) {
        static const DebugEnums<T>& instance = debugEnums;
        return qScriptRegisterMetaType<T>(
            engine,
            [](QScriptEngine* engine, const T& topology) -> QScriptValue {
                return instance.value(topology);
            },
            [](const QScriptValue& value, T& topology) {
                topology = instance.key(value.toString());
            }
        );
    }
}

void GraphicsScriptingInterface::registerMetaTypes(QScriptEngine* engine) {
    qScriptRegisterSequenceMetaType<QVector<glm::uint32>>(engine);
    qScriptRegisterSequenceMetaType<QVector<scriptable::ScriptableMaterialLayer>>(engine);

    scriptable::registerQPointerMetaType<scriptable::ScriptableModel>(engine);
    scriptable::registerQPointerMetaType<scriptable::ScriptableMesh>(engine);
    scriptable::registerQPointerMetaType<scriptable::ScriptableMeshPart>(engine);

    scriptable::registerDebugEnum<graphics::Mesh::Topology>(engine, graphics::TOPOLOGIES);
    scriptable::registerDebugEnum<gpu::Type>(engine, gpu::TYPES);
    scriptable::registerDebugEnum<gpu::Semantic>(engine, gpu::SEMANTICS);
    scriptable::registerDebugEnum<gpu::Dimension>(engine, gpu::DIMENSIONS);

    qScriptRegisterMetaType(engine, scriptable::scriptableMaterialToScriptValue, scriptable::scriptableMaterialFromScriptValue);
    qScriptRegisterMetaType(engine, scriptable::scriptableMaterialLayerToScriptValue, scriptable::scriptableMaterialLayerFromScriptValue);
    qScriptRegisterMetaType(engine, scriptable::qVectorScriptableMaterialLayerToScriptValue, scriptable::qVectorScriptableMaterialLayerFromScriptValue);
    qScriptRegisterMetaType(engine, scriptable::multiMaterialMapToScriptValue, scriptable::multiMaterialMapFromScriptValue);

    Q_UNUSED(metaTypeIds);
}
