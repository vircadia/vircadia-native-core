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
#include <QScriptEngine>
#include <QScriptValueIterator>
#include <QtScript/QScriptValue>
#include <model-networking/SimpleMeshProxy.h>
#include "ScriptEngine.h"
#include "ScriptEngineLogging.h"
#include "OBJWriter.h"

ModelScriptingInterface::ModelScriptingInterface(QObject* parent) : QObject(parent) {
    _modelScriptEngine = qobject_cast<QScriptEngine*>(parent);

    qScriptRegisterSequenceMetaType<QList<MeshProxy*>>(_modelScriptEngine);
    qScriptRegisterMetaType(_modelScriptEngine, meshFaceToScriptValue, meshFaceFromScriptValue);
    qScriptRegisterMetaType(_modelScriptEngine, qVectorMeshFaceToScriptValue, qVectorMeshFaceFromScriptValue);
}

QString ModelScriptingInterface::meshToOBJ(MeshProxyList in) {
    QList<MeshPointer> meshes;
    foreach (const MeshProxy* meshProxy, in) {
        meshes.append(meshProxy->getMeshPointer());
    }

    return writeOBJToString(meshes);
}

QScriptValue ModelScriptingInterface::appendMeshes(MeshProxyList in) {
    // figure out the size of the resulting mesh
    size_t totalVertexCount { 0 };
    size_t totalColorCount { 0 };
    size_t totalNormalCount { 0 };
    size_t totalIndexCount { 0 };
    foreach (const MeshProxy* meshProxy, in) {
        MeshPointer mesh = meshProxy->getMeshPointer();
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

    foreach (const MeshProxy* meshProxy, in) {
        MeshPointer mesh = meshProxy->getMeshPointer();
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

    model::MeshPointer result(new model::Mesh());

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

    std::vector<model::Mesh::Part> parts;
    parts.emplace_back(model::Mesh::Part((model::Index)0, // startIndex
                                         (model::Index)result->getNumIndices(), // numIndices
                                         (model::Index)0, // baseVertex
                                         model::Mesh::TRIANGLES)); // topology
    result->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(model::Mesh::Part),
                                                          (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));


    MeshProxy* resultProxy = new SimpleMeshProxy(result);
    return meshToScriptValue(_modelScriptEngine, resultProxy);
}

QScriptValue ModelScriptingInterface::transformMesh(glm::mat4 transform, MeshProxy* meshProxy) {
    if (!meshProxy) {
        return QScriptValue(false);
    }
    MeshPointer mesh = meshProxy->getMeshPointer();
    if (!mesh) {
        return QScriptValue(false);
    }

    model::MeshPointer result = mesh->map([&](glm::vec3 position){ return glm::vec3(transform * glm::vec4(position, 1.0f)); },
                                          [&](glm::vec3 color){ return color; },
                                          [&](glm::vec3 normal){ return glm::vec3(transform * glm::vec4(normal, 0.0f)); },
                                          [&](uint32_t index){ return index; });
    MeshProxy* resultProxy = new SimpleMeshProxy(result);
    return meshToScriptValue(_modelScriptEngine, resultProxy);
}

QScriptValue ModelScriptingInterface::getVertexCount(MeshProxy* meshProxy) {
    if (!meshProxy) {
        return QScriptValue(false);
    }
    MeshPointer mesh = meshProxy->getMeshPointer();
    if (!mesh) {
        return QScriptValue(false);
    }

    gpu::BufferView::Index numVertices = (gpu::BufferView::Index)mesh->getNumVertices();

    return numVertices;
}

QScriptValue ModelScriptingInterface::getVertex(MeshProxy* meshProxy, int vertexIndex) {
    if (!meshProxy) {
        return QScriptValue(false);
    }
    MeshPointer mesh = meshProxy->getMeshPointer();
    if (!mesh) {
        return QScriptValue(false);
    }

    const gpu::BufferView& vertexBufferView = mesh->getVertexBuffer();
    gpu::BufferView::Index numVertices = (gpu::BufferView::Index)mesh->getNumVertices();

    if (vertexIndex < 0 || vertexIndex >= numVertices) {
        return QScriptValue(false);
    }

    glm::vec3 pos = vertexBufferView.get<glm::vec3>(vertexIndex);
    return vec3toScriptValue(_modelScriptEngine, pos);
}


QScriptValue ModelScriptingInterface::newMesh(const QVector<glm::vec3>& vertices,
                                              const QVector<glm::vec3>& normals,
                                              const QVector<MeshFace>& faces) {
    model::MeshPointer mesh(new model::Mesh());

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
        qCDebug(scriptengine) << "ModelScriptingInterface::newMesh normals must be same length as vertices";
    }

    // indices (faces)
    int VERTICES_PER_TRIANGLE = 3;
    int indexBufferSize = faces.size() * sizeof(uint32_t) * VERTICES_PER_TRIANGLE;
    unsigned char* indexData  = new unsigned char[indexBufferSize];
    unsigned char* indexDataCursor = indexData;
    foreach(const MeshFace& meshFace, faces) {
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
    std::vector<model::Mesh::Part> parts;
    parts.emplace_back(model::Mesh::Part((model::Index)0, // startIndex
                                         (model::Index)faces.size() * 3, // numIndices
                                         (model::Index)0, // baseVertex
                                         model::Mesh::TRIANGLES)); // topology
    mesh->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(model::Mesh::Part),
                                                        (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));



    MeshProxy* meshProxy = new SimpleMeshProxy(mesh);
    return meshToScriptValue(_modelScriptEngine, meshProxy);
}
