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

#include <QScriptEngine>
#include <QScriptValueIterator>
#include <QtScript/QScriptValue>
#include "ScriptEngine.h"
#include "ModelScriptingInterface.h"
#include "OBJWriter.h"

ModelScriptingInterface::ModelScriptingInterface(QObject* parent) : QObject(parent) {
    _modelScriptEngine = qobject_cast<ScriptEngine*>(parent);
}

QScriptValue meshToScriptValue(QScriptEngine* engine, MeshProxy* const &in) {
    return engine->newQObject(in, QScriptEngine::QtOwnership,
                              QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects);
}

void meshFromScriptValue(const QScriptValue& value, MeshProxy* &out) {
    out = qobject_cast<MeshProxy*>(value.toQObject());
}

QScriptValue meshesToScriptValue(QScriptEngine* engine, const MeshProxyList &in) {
    return engine->toScriptValue(in);
}

void meshesFromScriptValue(const QScriptValue& value, MeshProxyList &out) {
    QScriptValueIterator itr(value);
    while(itr.hasNext()) {
        itr.next();
        MeshProxy* meshProxy = qscriptvalue_cast<MeshProxyList::value_type>(itr.value());
        if (meshProxy) {
            out.append(meshProxy);
        }
    }
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
    size_t totalAttributeCount { 0 };
    size_t totalIndexCount { 0 };
    foreach (const MeshProxy* meshProxy, in) {
        MeshPointer mesh = meshProxy->getMeshPointer();
        totalVertexCount += mesh->getNumVertices();

        int attributeTypeNormal = gpu::Stream::InputSlot::NORMAL; // libraries/gpu/src/gpu/Stream.h
        const gpu::BufferView& normalsBufferView = mesh->getAttributeBuffer(attributeTypeNormal);
        gpu::BufferView::Index numNormals = (gpu::BufferView::Index)normalsBufferView.getNumElements();
        totalAttributeCount += numNormals;

        totalIndexCount += mesh->getNumIndices();
    }

    // alloc the resulting mesh
    gpu::Resource::Size combinedVertexSize = totalVertexCount * sizeof(glm::vec3);
    unsigned char* combinedVertexData  = new unsigned char[combinedVertexSize];
    unsigned char* combinedVertexDataCursor = combinedVertexData;

    gpu::Resource::Size combinedNormalSize = totalAttributeCount * sizeof(glm::vec3);
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


    MeshProxy* resultProxy = new MeshProxy(result);
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
                                          [&](glm::vec3 normal){ return glm::vec3(transform * glm::vec4(normal, 0.0f)); },
                                          [&](uint32_t index){ return index; });
    MeshProxy* resultProxy = new MeshProxy(result);
    return meshToScriptValue(_modelScriptEngine, resultProxy);
}
