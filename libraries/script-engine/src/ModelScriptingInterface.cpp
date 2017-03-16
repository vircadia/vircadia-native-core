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
    model::MeshPointer result(new model::Mesh());
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

    gpu::Resource::Size combinedNormalSize = totalVertexCount * sizeof(glm::vec3);
    unsigned char* combinedNormalData  = new unsigned char[combinedNormalSize];
    unsigned char* combinedNormalDataCursor = combinedNormalData;

    gpu::Resource::Size combinedIndexSize = totalVertexCount * sizeof(uint32_t);
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
        gpu::BufferView::Index numNormals =  (gpu::BufferView::Index)mesh->getNumAttributes();
        for (gpu::BufferView::Index i = 0; i < numNormals; i ++) {
            glm::vec3 normal = normalsBufferView.get<glm::vec3>(i);
            memcpy(combinedNormalDataCursor, &normal, sizeof(normal));
            combinedNormalDataCursor += sizeof(normal);
        }
        // TODO -- other attributes

        // face data
        const gpu::BufferView& indexBufferView = mesh->getIndexBuffer();
        gpu::BufferView::Index numIndexes =  (gpu::BufferView::Index)mesh->getNumAttributes();
        for (gpu::BufferView::Index i = 0; i < numIndexes; i ++) {
            uint32_t index = indexBufferView.get<uint32_t>(i);
            index += indexStartOffset;
            memcpy(combinedIndexDataCursor, &index, sizeof(index));
            combinedIndexDataCursor += sizeof(index);
        }

        indexStartOffset += numVertices;
    }

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

    MeshProxy* resultProxy = new MeshProxy(result);
    return meshToScriptValue(_modelScriptEngine, resultProxy);
}
