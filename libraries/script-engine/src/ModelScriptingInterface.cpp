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
    int attributeTypeNormal = gpu::Stream::InputSlot::NORMAL; // libraries/gpu/src/gpu/Stream.h

    size_t totalVertexCount { 0 };
    size_t totalAttributeCount { 0 };
    size_t totalIndexCount { 0 };
    foreach (const MeshProxy* meshProxy, in) {
        MeshPointer mesh = meshProxy->getMeshPointer();
        totalVertexCount += mesh->getNumVertices();
        totalAttributeCount += mesh->getNumAttributes();
        totalIndexCount += mesh->getNumIndices();
    }

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

        // vertex data
        const gpu::BufferView& vertexBufferView = mesh->getVertexBuffer();
        gpu::BufferView::Index numVertices = (gpu::BufferView::Index)mesh->getNumVertices();
        for (gpu::BufferView::Index i = 0; i < numVertices; i ++) {
            glm::vec3 pos = vertexBufferView.get<glm::vec3>(i);
            memcpy(combinedVertexDataCursor, &pos, sizeof(pos));
            combinedVertexDataCursor += sizeof(pos);
        }

        // normal data
        const gpu::BufferView& normalsBufferView = mesh->getAttributeBuffer(attributeTypeNormal);
        gpu::BufferView::Index numNormals = (gpu::BufferView::Index)mesh->getNumAttributes();
        for (gpu::BufferView::Index i = 0; i < numNormals; i ++) {
            glm::vec3 normal = normalsBufferView.get<glm::vec3>(i);
            memcpy(combinedNormalDataCursor, &normal, sizeof(normal));
            combinedNormalDataCursor += sizeof(normal);
        }
        // TODO -- other attributes

        // face data
        const gpu::BufferView& indexBufferView = mesh->getIndexBuffer();
        gpu::BufferView::Index numIndexes = (gpu::BufferView::Index)mesh->getNumIndices();
        for (gpu::BufferView::Index i = 0; i < numIndexes; i ++) {
            uint32_t index = indexBufferView.get<uint32_t>(i);
            index += indexStartOffset;
            memcpy(combinedIndexDataCursor, &index, sizeof(index));
            combinedIndexDataCursor += sizeof(index);
        }

        indexStartOffset += numVertices;
    }

    model::MeshPointer result(new model::Mesh());

    gpu::Element vertexElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
    gpu::Buffer* combinedVertexBuffer = new gpu::Buffer(combinedVertexSize, combinedVertexData);
    gpu::BufferPointer combinedVertexBufferPointer(combinedVertexBuffer);
    gpu::BufferView combinedVertexBufferView(combinedVertexBufferPointer, vertexElement);
    result->setVertexBuffer(combinedVertexBufferView);

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


// QScriptValue ModelScriptingInterface::transformMesh(glm::mat4 transform, MeshProxy* meshProxy) {
//     int attributeTypeNormal = gpu::Stream::InputSlot::NORMAL; // libraries/gpu/src/gpu/Stream.h

//     if (!meshProxy) {
//         return QScriptValue(false);
//     }

//     MeshPointer mesh = meshProxy->getMeshPointer();

//     gpu::Resource::Size vertexSize = mesh->getNumVertices() * sizeof(glm::vec3);
//     unsigned char* resultVertexData  = new unsigned char[vertexSize];
//     unsigned char* vertexDataCursor = resultVertexData;

//     gpu::Resource::Size normalSize = mesh->getNumAttributes() * sizeof(glm::vec3);
//     unsigned char* resultNormalData  = new unsigned char[normalSize];
//     unsigned char* normalDataCursor = resultNormalData;

//     gpu::Resource::Size indexSize = mesh->getNumIndices() * sizeof(uint32_t);
//     unsigned char* resultIndexData  = new unsigned char[indexSize];
//     unsigned char* indexDataCursor = resultIndexData;

//     // vertex data
//     const gpu::BufferView& vertexBufferView = mesh->getVertexBuffer();
//     gpu::BufferView::Index numVertices = (gpu::BufferView::Index)mesh->getNumVertices();
//     for (gpu::BufferView::Index i = 0; i < numVertices; i ++) {
//         glm::vec3 pos = vertexBufferView.get<glm::vec3>(i);
//         pos = glm::vec3(transform * glm::vec4(pos, 1.0f));
//         memcpy(vertexDataCursor, &pos, sizeof(pos));
//         vertexDataCursor += sizeof(pos);
//     }

//     // normal data
//     const gpu::BufferView& normalsBufferView = mesh->getAttributeBuffer(attributeTypeNormal);
//     gpu::BufferView::Index numNormals =  (gpu::BufferView::Index)mesh->getNumAttributes();
//     for (gpu::BufferView::Index i = 0; i < numNormals; i ++) {
//         glm::vec3 normal = normalsBufferView.get<glm::vec3>(i);
//         normal = glm::vec3(transform * glm::vec4(normal, 0.0f));
//         memcpy(normalDataCursor, &normal, sizeof(normal));
//         normalDataCursor += sizeof(normal);
//     }
//     // TODO -- other attributes

//     // face data
//     const gpu::BufferView& indexBufferView = mesh->getIndexBuffer();
//     gpu::BufferView::Index numIndexes =  (gpu::BufferView::Index)mesh->getNumIndices();
//     for (gpu::BufferView::Index i = 0; i < numIndexes; i ++) {
//         uint32_t index = indexBufferView.get<uint32_t>(i);
//         memcpy(indexDataCursor, &index, sizeof(index));
//         indexDataCursor += sizeof(index);
//     }

//     model::MeshPointer result(new model::Mesh());

//     gpu::Element vertexElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
//     gpu::Buffer* resultVertexBuffer = new gpu::Buffer(vertexSize, resultVertexData);
//     gpu::BufferPointer resultVertexBufferPointer(resultVertexBuffer);
//     gpu::BufferView resultVertexBufferView(resultVertexBufferPointer, vertexElement);
//     result->setVertexBuffer(resultVertexBufferView);

//     gpu::Element normalElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
//     gpu::Buffer* resultNormalsBuffer = new gpu::Buffer(normalSize, resultNormalData);
//     gpu::BufferPointer resultNormalsBufferPointer(resultNormalsBuffer);
//     gpu::BufferView resultNormalsBufferView(resultNormalsBufferPointer, normalElement);
//     result->addAttribute(attributeTypeNormal, resultNormalsBufferView);

//     gpu::Element indexElement = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW);
//     gpu::Buffer* resultIndexesBuffer = new gpu::Buffer(indexSize, resultIndexData);
//     gpu::BufferPointer resultIndexesBufferPointer(resultIndexesBuffer);
//     gpu::BufferView resultIndexesBufferView(resultIndexesBufferPointer, indexElement);
//     result->setIndexBuffer(resultIndexesBufferView);


//     std::vector<model::Mesh::Part> parts;
//     parts.emplace_back(model::Mesh::Part((model::Index)0, // startIndex
//                                          (model::Index)result->getNumIndices(), // numIndices
//                                          (model::Index)0, // baseVertex
//                                          model::Mesh::TRIANGLES)); // topology
//     result->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(model::Mesh::Part),
//                                                           (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));



//     MeshProxy* resultProxy = new MeshProxy(result);
//     return meshToScriptValue(_modelScriptEngine, resultProxy);
// }


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
