//
//  ModelScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelScriptingInterface.h"
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValueIterator>
#include <QtScript/QScriptValue>
#include <QUuid>
#include "BaseScriptEngine.h"
#include "ScriptEngineLogging.h"
#include "OBJWriter.h"

#include <GeometryUtil.h>
#include <shared/QtHelpers.h>

#include <graphics-scripting/DebugNames.h>
#include <graphics-scripting/BufferViewHelpers.h>

#include "BufferViewScripting.h"
#include "ScriptableMesh.h"
#include "GraphicsScriptingUtil.h"

#include "ModelScriptingInterface.moc"

#include "RegisteredMetaTypes.h"

ModelScriptingInterface::ModelScriptingInterface(QObject* parent) : QObject(parent) {
    if (auto scriptEngine = qobject_cast<QScriptEngine*>(parent)) {
        this->registerMetaTypes(scriptEngine);
    }
}

bool ModelScriptingInterface::updateMeshes(QUuid uuid, const scriptable::ScriptableMeshPointer mesh, int meshIndex, int partIndex) {
    auto model = scriptable::make_qtowned<scriptable::ScriptableModel>();
    if (mesh) {
        model->append(*mesh);
    }
    return updateMeshes(uuid, model.get());
}

bool ModelScriptingInterface::updateMeshes(QUuid uuid, const scriptable::ScriptableModelPointer model) {
    auto appProvider = DependencyManager::get<scriptable::ModelProviderFactory>();
    //qCDebug(graphics_scripting) << "appProvider" << appProvider.data();
    scriptable::ModelProviderPointer provider = appProvider ? appProvider->lookupModelProvider(uuid) : nullptr;
    QString providerType = provider ? provider->metadata.value("providerType").toString() : QString();
    if (providerType.isEmpty()) {
        providerType = "unknown";
    }
    bool success = false;
    if (provider) {
        //qCDebug(graphics_scripting) << "fetching meshes from " << providerType << "...";
        auto scriptableMeshes = provider->getScriptableModel(&success);
        //qCDebug(graphics_scripting) << "//fetched meshes from " << providerType << "success:" <<success << "#" << scriptableMeshes.meshes.size();
        if (success) {
            const scriptable::ScriptableModelBasePointer base = model->operator scriptable::ScriptableModelBasePointer();
            //qCDebug(graphics_scripting) << "as base" << base;
            if (base) {
                //auto meshes = model->getConstMeshes();
                success = provider->replaceScriptableModelMeshPart(base, -1, -1);
                
                // for (uint32_t m = 0; success && m < meshes.size(); m++) {
                //     const auto& mesh = meshes.at(m);
                //     for (int p = 0; success && p < mesh->getNumParts(); p++) {
                //         qCDebug(graphics_scripting) << "provider->replaceScriptableModelMeshPart" << "meshIndex" << m << "partIndex" << p;
                //         success = provider->replaceScriptableModelMeshPart(base, m, p);
                //         //if (!success) {
                //         qCDebug(graphics_scripting) << "//provider->replaceScriptableModelMeshPart" << "meshIndex" << m << "partIndex" << p << success;
                //     }
                // }
            }
        }
    }
    return success;
}

void ModelScriptingInterface::getMeshes(QUuid uuid, QScriptValue callback) {
    auto handler = scriptable::jsBindCallback(callback);
    Q_ASSERT(handler.engine() == this->engine());
    QPointer<BaseScriptEngine> engine = dynamic_cast<BaseScriptEngine*>(handler.engine());

    scriptable::ScriptableModel* meshes{ nullptr };
    bool success = false;
    QString error;

    auto appProvider = DependencyManager::get<scriptable::ModelProviderFactory>();
    qCDebug(graphics_scripting) << "appProvider" << appProvider.data();
    scriptable::ModelProviderPointer provider = appProvider ? appProvider->lookupModelProvider(uuid) : nullptr;
    QString providerType = provider ? provider->metadata.value("providerType").toString() : QString();
    if (providerType.isEmpty()) {
        providerType = "unknown";
    }
    if (provider) {
        qCDebug(graphics_scripting) << "fetching meshes from " << providerType << "...";
        auto scriptableMeshes = provider->getScriptableModel(&success);
        qCDebug(graphics_scripting) << "//fetched meshes from " << providerType << "success:" <<success << "#" << scriptableMeshes.meshes.size();
        if (success) {
            meshes = scriptable::make_scriptowned<scriptable::ScriptableModel>(scriptableMeshes);
        QString debugString = scriptable::toDebugString(meshes);
        QObject::connect(meshes, &QObject::destroyed, this, [=]() {
                qCDebug(graphics_scripting) << "///fetched meshes" << debugString;
            });
        
            if (meshes->objectName().isEmpty()) {
                meshes->setObjectName(providerType+"::meshes");
            }
            if (meshes->objectID.isNull()) {
                meshes->objectID = uuid.toString();
            }
            meshes->metadata["provider"] = provider->metadata;
        }
    }
    if (!success) {
        error = QString("failed to get meshes from %1 provider for uuid %2").arg(providerType).arg(uuid.toString());
    }

    if (!error.isEmpty()) {
        qCWarning(graphics_scripting) << "ModelScriptingInterface::getMeshes ERROR" << error;
        callScopedHandlerObject(handler, engine->makeError(error), QScriptValue::NullValue);
    } else {
        callScopedHandlerObject(handler, QScriptValue::NullValue, engine->toScriptValue(meshes));
    }
}

QString ModelScriptingInterface::meshToOBJ(const scriptable::ScriptableModel& _in) {
    const auto& in = _in.getConstMeshes();
    qCDebug(graphics_scripting) << "meshToOBJ" << in.size();
    if (in.size()) {
        QList<scriptable::MeshPointer> meshes;
        foreach (auto meshProxy, in) {
            qCDebug(graphics_scripting) << "meshToOBJ" << meshProxy;
            if (meshProxy) {
                meshes.append(getMeshPointer(meshProxy));
            }
        }
        if (meshes.size()) {
            return writeOBJToString(meshes);
        }
    }
    context()->throwError(QString("null mesh"));
    return QString();
}

QScriptValue ModelScriptingInterface::appendMeshes(scriptable::ScriptableModel _in) {
    const auto& in = _in.getMeshes();

    // figure out the size of the resulting mesh
    size_t totalVertexCount { 0 };
    size_t totalColorCount { 0 };
    size_t totalNormalCount { 0 };
    size_t totalIndexCount { 0 };
    foreach (auto& meshProxy, in) {
        scriptable::MeshPointer mesh = getMeshPointer(meshProxy);
        totalVertexCount += mesh->getNumVertices();

        int attributeTypeColor = gpu::Stream::InputSlot::COLOR; // libraries/gpu/src/gpu/Stream.h
        const gpu::BufferView& colorsBufferView = mesh->getAttributeBuffer(attributeTypeColor);
        gpu::BufferView::Index numColors = (gpu::BufferView::Index)colorsBufferView.getNumElements();
        totalColorCount += numColors;

        int attributeTypeNormal = gpu::Stream::InputSlot::NORMAL; // libraries/gpu/src/gpu/Stream.h
        const gpu::BufferView& normalsBufferView = mesh->getAttributeBuffer(attributeTypeNormal);
        gpu::BufferView::Index numNormals = (gpu::BufferView::Index)normalsBufferView.getNumElements();
        totalNormalCount += numNormals;

        totalIndexCount += mesh->getNumIndices();
    }

    // alloc the resulting mesh
    gpu::Resource::Size combinedVertexSize = totalVertexCount * sizeof(glm::vec3);
    unsigned char* combinedVertexData  = new unsigned char[combinedVertexSize];
    unsigned char* combinedVertexDataCursor = combinedVertexData;

    gpu::Resource::Size combinedColorSize = totalColorCount * sizeof(glm::vec3);
    unsigned char* combinedColorData  = new unsigned char[combinedColorSize];
    unsigned char* combinedColorDataCursor = combinedColorData;

    gpu::Resource::Size combinedNormalSize = totalNormalCount * sizeof(glm::vec3);
    unsigned char* combinedNormalData  = new unsigned char[combinedNormalSize];
    unsigned char* combinedNormalDataCursor = combinedNormalData;

    gpu::Resource::Size combinedIndexSize = totalIndexCount * sizeof(uint32_t);
    unsigned char* combinedIndexData  = new unsigned char[combinedIndexSize];
    unsigned char* combinedIndexDataCursor = combinedIndexData;

    uint32_t indexStartOffset { 0 };

    foreach (const auto& meshProxy, in) {
        scriptable::MeshPointer mesh = getMeshPointer(meshProxy);
        mesh->forEach(
            [&](glm::vec3 position){
                memcpy(combinedVertexDataCursor, &position, sizeof(position));
                combinedVertexDataCursor += sizeof(position);
            },
            [&](glm::vec3 color){
                memcpy(combinedColorDataCursor, &color, sizeof(color));
                combinedColorDataCursor += sizeof(color);
            },
            [&](glm::vec3 normal){
                memcpy(combinedNormalDataCursor, &normal, sizeof(normal));
                combinedNormalDataCursor += sizeof(normal);
            },
            [&](uint32_t index){
                index += indexStartOffset;
                memcpy(combinedIndexDataCursor, &index, sizeof(index));
                combinedIndexDataCursor += sizeof(index);
            });

        gpu::BufferView::Index numVertices = (gpu::BufferView::Index)mesh->getNumVertices();
        indexStartOffset += numVertices;
    }

    graphics::MeshPointer result(new graphics::Mesh());

    gpu::Element vertexElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
    gpu::Buffer* combinedVertexBuffer = new gpu::Buffer(combinedVertexSize, combinedVertexData);
    gpu::BufferPointer combinedVertexBufferPointer(combinedVertexBuffer);
    gpu::BufferView combinedVertexBufferView(combinedVertexBufferPointer, vertexElement);
    result->setVertexBuffer(combinedVertexBufferView);

    int attributeTypeColor = gpu::Stream::InputSlot::COLOR; // libraries/gpu/src/gpu/Stream.h
    gpu::Element colorElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
    gpu::Buffer* combinedColorsBuffer = new gpu::Buffer(combinedColorSize, combinedColorData);
    gpu::BufferPointer combinedColorsBufferPointer(combinedColorsBuffer);
    gpu::BufferView combinedColorsBufferView(combinedColorsBufferPointer, colorElement);
    result->addAttribute(attributeTypeColor, combinedColorsBufferView);

    int attributeTypeNormal = gpu::Stream::InputSlot::NORMAL; // libraries/gpu/src/gpu/Stream.h
    gpu::Element normalElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
    gpu::Buffer* combinedNormalsBuffer = new gpu::Buffer(combinedNormalSize, combinedNormalData);
    gpu::BufferPointer combinedNormalsBufferPointer(combinedNormalsBuffer);
    gpu::BufferView combinedNormalsBufferView(combinedNormalsBufferPointer, normalElement);
    result->addAttribute(attributeTypeNormal, combinedNormalsBufferView);

    gpu::Element indexElement = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW);
    gpu::Buffer* combinedIndexesBuffer = new gpu::Buffer(combinedIndexSize, combinedIndexData);
    gpu::BufferPointer combinedIndexesBufferPointer(combinedIndexesBuffer);
    gpu::BufferView combinedIndexesBufferView(combinedIndexesBufferPointer, indexElement);
    result->setIndexBuffer(combinedIndexesBufferView);

    std::vector<graphics::Mesh::Part> parts;
    parts.emplace_back(graphics::Mesh::Part((graphics::Index)0, // startIndex
                                         (graphics::Index)result->getNumIndices(), // numIndices
                                         (graphics::Index)0, // baseVertex
                                         graphics::Mesh::TRIANGLES)); // topology
    result->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(graphics::Mesh::Part),
                                                          (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));


    return engine()->toScriptValue(scriptable::make_scriptowned<scriptable::ScriptableMesh>(result));
}

QScriptValue ModelScriptingInterface::transformMesh(scriptable::ScriptableMeshPointer meshProxy, glm::mat4 transform) {
    auto mesh = getMeshPointer(meshProxy);
    if (!mesh) {
        return false;
    }

    graphics::MeshPointer result = mesh->map([&](glm::vec3 position){ return glm::vec3(transform * glm::vec4(position, 1.0f)); },
                                          [&](glm::vec3 color){ return color; },
                                          [&](glm::vec3 normal){ return glm::vec3(transform * glm::vec4(normal, 0.0f)); },
                                          [&](uint32_t index){ return index; });
    return engine()->toScriptValue(scriptable::make_scriptowned<scriptable::ScriptableMesh>(result));
}

QScriptValue ModelScriptingInterface::getVertexCount(scriptable::ScriptableMeshPointer meshProxy) {
    auto mesh = getMeshPointer(meshProxy);
    if (!mesh) {
        return -1;
    }
    return (uint32_t)mesh->getNumVertices();
}

QScriptValue ModelScriptingInterface::getVertex(scriptable::ScriptableMeshPointer meshProxy, quint32 vertexIndex) {
    auto mesh = getMeshPointer(meshProxy);
    if (!mesh) {
        return QScriptValue();
    }

    const gpu::BufferView& vertexBufferView = mesh->getVertexBuffer();
    auto numVertices = mesh->getNumVertices();

    if (vertexIndex >= numVertices) {
        context()->throwError(QString("invalid index: %1 [0,%2)").arg(vertexIndex).arg(numVertices));
        return QScriptValue::NullValue;
    }

    glm::vec3 pos = vertexBufferView.get<glm::vec3>(vertexIndex);
    return engine()->toScriptValue(pos);
}

QScriptValue ModelScriptingInterface::newMesh(const QVector<glm::vec3>& vertices,
                                              const QVector<glm::vec3>& normals,
                                              const QVector<mesh::MeshFace>& faces) {
    graphics::MeshPointer mesh(new graphics::Mesh());

    // vertices
    auto vertexBuffer = std::make_shared<gpu::Buffer>(vertices.size() * sizeof(glm::vec3), (gpu::Byte*)vertices.data());
    auto vertexBufferPtr = gpu::BufferPointer(vertexBuffer);
    gpu::BufferView vertexBufferView(vertexBufferPtr, 0, vertexBufferPtr->getSize(),
                                     sizeof(glm::vec3), gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
    mesh->setVertexBuffer(vertexBufferView);

    if (vertices.size() == normals.size()) {
        // normals
        auto normalBuffer = std::make_shared<gpu::Buffer>(normals.size() * sizeof(glm::vec3), (gpu::Byte*)normals.data());
        auto normalBufferPtr = gpu::BufferPointer(normalBuffer);
        gpu::BufferView normalBufferView(normalBufferPtr, 0, normalBufferPtr->getSize(),
                                         sizeof(glm::vec3), gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
        mesh->addAttribute(gpu::Stream::NORMAL, normalBufferView);
    } else {
        qCWarning(graphics_scripting, "ModelScriptingInterface::newMesh normals must be same length as vertices");
    }

    // indices (faces)
    int VERTICES_PER_TRIANGLE = 3;
    int indexBufferSize = faces.size() * sizeof(uint32_t) * VERTICES_PER_TRIANGLE;
    unsigned char* indexData  = new unsigned char[indexBufferSize];
    unsigned char* indexDataCursor = indexData;
    foreach(const mesh::MeshFace& meshFace, faces) {
        for (int i = 0; i < VERTICES_PER_TRIANGLE; i++) {
            memcpy(indexDataCursor, &meshFace.vertexIndices[i], sizeof(uint32_t));
            indexDataCursor += sizeof(uint32_t);
        }
    }
    auto indexBuffer = std::make_shared<gpu::Buffer>(indexBufferSize, (gpu::Byte*)indexData);
    auto indexBufferPtr = gpu::BufferPointer(indexBuffer);
    gpu::BufferView indexBufferView(indexBufferPtr, gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW));
    mesh->setIndexBuffer(indexBufferView);

    // parts
    std::vector<graphics::Mesh::Part> parts;
    parts.emplace_back(graphics::Mesh::Part((graphics::Index)0, // startIndex
                                         (graphics::Index)faces.size() * 3, // numIndices
                                         (graphics::Index)0, // baseVertex
                                         graphics::Mesh::TRIANGLES)); // topology
    mesh->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(graphics::Mesh::Part),
                                                        (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));



    return engine()->toScriptValue(scriptable::make_scriptowned<scriptable::ScriptableMesh>(mesh));
}

namespace {

    // FIXME: MESHFACES:
    // QScriptValue meshFaceToScriptValue(QScriptEngine* engine, const mesh::MeshFace &meshFace) {
    //     QScriptValue obj = engine->newObject();
    //     obj.setProperty("vertices", qVectorIntToScriptValue(engine, meshFace.vertexIndices));
    //     return obj;
    // }
    // void meshFaceFromScriptValue(const QScriptValue &object, mesh::MeshFace& meshFaceResult) {
    //     qScriptValueToSequence(object.property("vertices"), meshFaceResult.vertexIndices);
    // }
    // QScriptValue qVectorMeshFaceToScriptValue(QScriptEngine* engine, const QVector<mesh::MeshFace>& vector) {
    //     return qScriptValueFromSequence(engine, vector);
    // }
    // void qVectorMeshFaceFromScriptValue(const QScriptValue& array, QVector<mesh::MeshFace>& result) {
    //     qScriptValueToSequence(array, result);
    // }

}


void ModelScriptingInterface::registerMetaTypes(QScriptEngine* engine) {
    scriptable::registerMetaTypes(engine);
    // FIXME: MESHFACES: remove if MeshFace is not needed anywhere
    // qScriptRegisterSequenceMetaType<mesh::MeshFaces>(engine);
    // qScriptRegisterMetaType(engine, meshFaceToScriptValue, meshFaceFromScriptValue);
    // qScriptRegisterMetaType(engine, qVectorMeshFaceToScriptValue, qVectorMeshFaceFromScriptValue);
}

MeshPointer ModelScriptingInterface::getMeshPointer(const scriptable::ScriptableMesh& meshProxy) {
    return meshProxy.getMeshPointer();
}
MeshPointer ModelScriptingInterface::getMeshPointer(scriptable::ScriptableMesh& meshProxy) {
    return getMeshPointer(&meshProxy);
}
MeshPointer ModelScriptingInterface::getMeshPointer(scriptable::ScriptableMeshPointer meshProxy) {
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
