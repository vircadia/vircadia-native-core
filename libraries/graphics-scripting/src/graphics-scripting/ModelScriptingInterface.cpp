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
#include "OBJReader.h"
//#include "ui/overlays/Base3DOverlay.h"
//#include "EntityTreeRenderer.h"
//#include "avatar/AvatarManager.h"
//#include "RenderableEntityItem.h"

#include <glm/gtx/string_cast.hpp>
#include <GeometryUtil.h>

#include <shared/QtHelpers.h>


#include <graphics-scripting/DebugNames.h>

#include <graphics-scripting/BufferViewHelpers.h>
#include "BufferViewScripting.h"

#include "ScriptableMesh.h"

using ScriptableMesh = scriptable::ScriptableMesh;

#include "ModelScriptingInterface.moc"

namespace {
    QLoggingCategory model_scripting { "hifi.model.scripting" };
}

ModelScriptingInterface::ModelScriptingInterface(QObject* parent) : QObject(parent) {
    if (auto scriptEngine = qobject_cast<QScriptEngine*>(parent)) {
        this->registerMetaTypes(scriptEngine);
    }
}

QString ModelScriptingInterface::meshToOBJ(const scriptable::ScriptableModel& _in) {
    const auto& in = _in.getMeshes();
    qCDebug(model_scripting) << "meshToOBJ" << in.size();
    if (in.size()) {
        QList<scriptable::MeshPointer> meshes;
        foreach (const auto meshProxy, in) {
            qCDebug(model_scripting) << "meshToOBJ" << meshProxy.get();
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
    foreach (const scriptable::ScriptableMeshPointer meshProxy, in) {
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

    foreach (const scriptable::ScriptableMeshPointer meshProxy, in) {
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


    scriptable::ScriptableMeshPointer resultProxy = scriptable::ScriptableMeshPointer(new ScriptableMesh(nullptr, result));
    return engine()->toScriptValue(result);
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
    scriptable::ScriptableMeshPointer resultProxy = scriptable::ScriptableMeshPointer(new ScriptableMesh(nullptr, result));
    return engine()->toScriptValue(resultProxy);
}

QScriptValue ModelScriptingInterface::getVertexCount(scriptable::ScriptableMeshPointer meshProxy) {
    auto mesh = getMeshPointer(meshProxy);
    if (!mesh) {
        return -1;
    }
    return (uint32_t)mesh->getNumVertices();
}

QScriptValue ModelScriptingInterface::getVertex(scriptable::ScriptableMeshPointer meshProxy, mesh::uint32 vertexIndex) {
    auto mesh = getMeshPointer(meshProxy);

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
        qCWarning(model_scripting, "ModelScriptingInterface::newMesh normals must be same length as vertices");
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



    scriptable::ScriptableMeshPointer meshProxy = scriptable::ScriptableMeshPointer(new ScriptableMesh(nullptr, mesh));
    return engine()->toScriptValue(meshProxy);
}

QScriptValue ModelScriptingInterface::mapAttributeValues(
    QScriptValue _in,
    QScriptValue scopeOrCallback,
    QScriptValue methodOrName
    ) {
    qCInfo(model_scripting) << "mapAttributeValues" << _in.toVariant().typeName() << _in.toVariant().toString() << _in.toQObject();
    auto in = qscriptvalue_cast<scriptable::ScriptableModel>(_in).getMeshes();
    if (in.size()) {
        foreach (scriptable::ScriptableMeshPointer meshProxy, in) {
            mapMeshAttributeValues(meshProxy, scopeOrCallback, methodOrName);
        }
        return thisObject();
    } else if (auto meshProxy = qobject_cast<scriptable::ScriptableMesh*>(_in.toQObject())) {
        return mapMeshAttributeValues(meshProxy->shared_from_this(), scopeOrCallback, methodOrName);
    } else {
        context()->throwError("invalid ModelProxy || MeshProxyPointer");
    }
    return false;
}


QScriptValue ModelScriptingInterface::unrollVertices(scriptable::ScriptableMeshPointer meshProxy, bool recalcNormals) {
    auto mesh = getMeshPointer(meshProxy);
    qCInfo(model_scripting) << "ModelScriptingInterface::unrollVertices" << !!mesh<< !!meshProxy;
    if (!mesh) {
        return QScriptValue();
    }

    auto positions = mesh->getVertexBuffer();
    auto indices = mesh->getIndexBuffer();
    quint32 numPoints = (quint32)indices.getNumElements();
    auto buffer = new gpu::Buffer();
    buffer->resize(numPoints * sizeof(uint32_t));
    auto newindices = gpu::BufferView(buffer, { gpu::SCALAR, gpu::UINT32, gpu::INDEX });
    qCInfo(model_scripting) << "ModelScriptingInterface::unrollVertices numPoints" << numPoints;
    auto attributeViews = ScriptableMesh::gatherBufferViews(mesh);
    for (const auto& a : attributeViews) {
        auto& view = a.second;
        auto sz = view._element.getSize();
        auto buffer = new gpu::Buffer();
        buffer->resize(numPoints * sz);
        auto points = gpu::BufferView(buffer, view._element);
        auto src = (uint8_t*)view._buffer->getData();
        auto dest = (uint8_t*)points._buffer->getData();
        auto slot = ScriptableMesh::ATTRIBUTES[a.first];
        qCInfo(model_scripting) << "ModelScriptingInterface::unrollVertices buffer" << a.first;
        qCInfo(model_scripting) << "ModelScriptingInterface::unrollVertices source" << view.getNumElements();
        qCInfo(model_scripting) << "ModelScriptingInterface::unrollVertices dest" << points.getNumElements();
        qCInfo(model_scripting) << "ModelScriptingInterface::unrollVertices sz" << sz << src << dest << slot;
        auto esize = indices._element.getSize();
        const char* hint= a.first.toStdString().c_str();
        for(quint32 i = 0; i < numPoints; i++) {
            quint32 index = esize == 4 ? indices.get<quint32>(i) : indices.get<quint16>(i);
            newindices.edit<uint32_t>(i) = i;
            bufferViewElementFromVariant(
                points, i,
                bufferViewElementToVariant(view, index, false, hint)
                    );
        }
        if (slot == gpu::Stream::POSITION) {
            mesh->setVertexBuffer(points);
        } else {
            mesh->addAttribute(slot, points);
        }
    }
    mesh->setIndexBuffer(newindices);
    if (recalcNormals) {
        recalculateNormals(meshProxy);
    }
    return true;
}

namespace {
    template <typename T>
    gpu::BufferView bufferViewFromVector(QVector<T> elements, gpu::Element elementType) {
        auto vertexBuffer = std::make_shared<gpu::Buffer>(
            elements.size() * sizeof(T),
            (gpu::Byte*)elements.data()
        );
        return { vertexBuffer, 0, vertexBuffer->getSize(),sizeof(T), elementType };
    }

    gpu::BufferView cloneBufferView(const gpu::BufferView& input) {
        //qCInfo(model_scripting) << "input" << input.getNumElements() << input._buffer->getSize();
        auto output = gpu::BufferView(
            std::make_shared<gpu::Buffer>(input._buffer->getSize(), input._buffer->getData()),
            input._offset,
            input._size,
            input._stride,
            input._element
            );
        //qCInfo(model_scripting) << "after" << output.getNumElements() << output._buffer->getSize();
        return output;
    }

    gpu::BufferView resizedBufferView(const gpu::BufferView& input, quint32 numElements) {
        auto effectiveSize = input._buffer->getSize() / input.getNumElements();
        qCInfo(model_scripting) << "resize input" << input.getNumElements() << input._buffer->getSize() << "effectiveSize" << effectiveSize;
        auto vsize = input._element.getSize() * numElements;
        gpu::Byte *data = new gpu::Byte[vsize];
        memset(data, 0, vsize);
        auto buffer = new gpu::Buffer(vsize, (gpu::Byte*)data);
        delete[] data;
        auto output = gpu::BufferView(buffer, input._element);
        qCInfo(model_scripting) << "resized output" << output.getNumElements() << output._buffer->getSize();
        return output;
    }
}

bool ModelScriptingInterface::replaceMeshData(scriptable::ScriptableMeshPointer dest, scriptable::ScriptableMeshPointer src, const QVector<QString>& attributeNames) {
    auto target = getMeshPointer(dest);
    auto source = getMeshPointer(src);
    if (!target || !source) {
        context()->throwError("ModelScriptingInterface::replaceMeshData -- expected dest and src to be valid mesh proxy pointers");
        return false;
    }

    QVector<QString> attributes = attributeNames.isEmpty() ? src->getAttributeNames() : attributeNames;

    //qCInfo(model_scripting) << "ModelScriptingInterface::replaceMeshData -- source:" << source->displayName << "target:" << target->displayName << "attributes:" << attributes;

    // remove attributes only found on target mesh, unless user has explicitly specified the relevant attribute names
    if (attributeNames.isEmpty()) {
        auto attributeViews = ScriptableMesh::gatherBufferViews(target);
        for (const auto& a : attributeViews) {
            auto slot = ScriptableMesh::ATTRIBUTES[a.first];
            if (!attributes.contains(a.first)) {
                //qCInfo(model_scripting) << "ModelScriptingInterface::replaceMeshData -- pruning target attribute" << a.first << slot;
                target->removeAttribute(slot);
            }
        }
    }

    target->setVertexBuffer(cloneBufferView(source->getVertexBuffer()));
    target->setIndexBuffer(cloneBufferView(source->getIndexBuffer()));
    target->setPartBuffer(cloneBufferView(source->getPartBuffer()));

    for (const auto& a : attributes) {
        auto slot = ScriptableMesh::ATTRIBUTES[a];
        if (slot == gpu::Stream::POSITION) {
            continue;
        }
        // auto& before = target->getAttributeBuffer(slot);
        auto& input = source->getAttributeBuffer(slot);
        if (input.getNumElements() == 0) {
            //qCInfo(model_scripting) << "ModelScriptingInterface::replaceMeshData buffer is empty -- pruning" << a << slot;
            target->removeAttribute(slot);
        } else {
            // if (before.getNumElements() == 0) {
            //     qCInfo(model_scripting) << "ModelScriptingInterface::replaceMeshData target buffer is empty -- adding" << a << slot;
            // } else {
            //     qCInfo(model_scripting) << "ModelScriptingInterface::replaceMeshData target buffer exists -- updating" << a << slot;
            // }
            target->addAttribute(slot, cloneBufferView(input));
        }
        // auto& after = target->getAttributeBuffer(slot);
        // qCInfo(model_scripting) << "ModelScriptingInterface::replaceMeshData" << a << slot << before.getNumElements() << " -> " << after.getNumElements();
    }


    return true;
}

bool ModelScriptingInterface::dedupeVertices(scriptable::ScriptableMeshPointer meshProxy, float epsilon) {
    auto mesh = getMeshPointer(meshProxy);
    if (!mesh) {
        return false;
    }
    auto positions = mesh->getVertexBuffer();
    auto numPositions = positions.getNumElements();
    const auto epsilon2 = epsilon*epsilon;

    QVector<glm::vec3> uniqueVerts;
    uniqueVerts.reserve((int)numPositions);
    QMap<quint32,quint32> remapIndices;

    for (quint32 i = 0; i < numPositions; i++) {
        const quint32 numUnique = uniqueVerts.size();
        const auto& position = positions.get<glm::vec3>(i);
        bool unique = true;
        for (quint32 j = 0; j < numUnique; j++) {
            if (glm::length2(uniqueVerts[j] - position) <= epsilon2) {
                remapIndices[i] = j;
                unique = false;
                break;
            }
        }
        if (unique) {
            uniqueVerts << position;
            remapIndices[i] = numUnique;
        }
    }

    qCInfo(model_scripting) << "//VERTS before" << numPositions << "after" << uniqueVerts.size();

    auto indices = mesh->getIndexBuffer();
    auto numIndices = indices.getNumElements();
    auto esize = indices._element.getSize();
    QVector<quint32> newIndices;
    newIndices.reserve((int)numIndices);
    for (quint32 i = 0; i < numIndices; i++) {
        quint32 index = esize == 4 ? indices.get<quint32>(i) : indices.get<quint16>(i);
        if (remapIndices.contains(index)) {
            //qCInfo(model_scripting) << i << index << "->" << remapIndices[index];
            newIndices << remapIndices[index];
        } else {
            qCInfo(model_scripting) << i << index << "!remapIndices[index]";
        }
    }

    mesh->setIndexBuffer(bufferViewFromVector(newIndices, { gpu::SCALAR, gpu::UINT32, gpu::INDEX }));
    mesh->setVertexBuffer(bufferViewFromVector(uniqueVerts, { gpu::VEC3, gpu::FLOAT, gpu::XYZ }));

    auto attributeViews = ScriptableMesh::gatherBufferViews(mesh);
    quint32 numUniqueVerts = uniqueVerts.size();
    for (const auto& a : attributeViews) {
        auto& view = a.second;
        auto slot = ScriptableMesh::ATTRIBUTES[a.first];
        if (slot == gpu::Stream::POSITION) {
            continue;
        }
        qCInfo(model_scripting) << "ModelScriptingInterface::dedupeVertices" << a.first << slot << view.getNumElements();
        auto newView = resizedBufferView(view, numUniqueVerts);
        qCInfo(model_scripting) << a.first << "before: #" << view.getNumElements() << "after: #" << newView.getNumElements();
        quint32 numElements = (quint32)view.getNumElements();
        for (quint32 i = 0; i < numElements; i++) {
            quint32 fromVertexIndex = i;
            quint32 toVertexIndex = remapIndices.contains(fromVertexIndex) ? remapIndices[fromVertexIndex] : fromVertexIndex;
            bufferViewElementFromVariant(
                newView, toVertexIndex,
                bufferViewElementToVariant(view, fromVertexIndex, false, "dedupe")
                );
        }
        mesh->addAttribute(slot, newView);
    }
    return true;
}

QScriptValue ModelScriptingInterface::cloneMesh(scriptable::ScriptableMeshPointer meshProxy, bool recalcNormals) {
    auto mesh = getMeshPointer(meshProxy);
    if (!mesh) {
        return QScriptValue::NullValue;
    }
    graphics::MeshPointer clone(new graphics::Mesh());
    clone->displayName = mesh->displayName + "-clone";
    qCInfo(model_scripting) << "ModelScriptingInterface::cloneMesh" << !!mesh<< !!meshProxy;
    if (!mesh) {
        return QScriptValue::NullValue;
    }

    clone->setIndexBuffer(cloneBufferView(mesh->getIndexBuffer()));
    clone->setPartBuffer(cloneBufferView(mesh->getPartBuffer()));
    auto attributeViews = ScriptableMesh::gatherBufferViews(mesh);
    for (const auto& a : attributeViews) {
        auto& view = a.second;
        auto slot = ScriptableMesh::ATTRIBUTES[a.first];
        qCInfo(model_scripting) << "ModelScriptingInterface::cloneVertices buffer" << a.first << slot;
        auto points = cloneBufferView(view);
        qCInfo(model_scripting) << "ModelScriptingInterface::cloneVertices source" << view.getNumElements();
        qCInfo(model_scripting) << "ModelScriptingInterface::cloneVertices dest" << points.getNumElements();
        if (slot == gpu::Stream::POSITION) {
            clone->setVertexBuffer(points);
        } else {
            clone->addAttribute(slot, points);
        }
    }

    auto result = scriptable::ScriptableMeshPointer(new ScriptableMesh(nullptr, clone));
    if (recalcNormals) {
        recalculateNormals(result);
    }
    return engine()->toScriptValue(result);
}

bool ModelScriptingInterface::recalculateNormals(scriptable::ScriptableMeshPointer meshProxy) {
    qCInfo(model_scripting) << "Recalculating normals" << !!meshProxy;
    auto mesh = getMeshPointer(meshProxy);
    if (!mesh) {
        return false;
    }
    ScriptableMesh::gatherBufferViews(mesh, { "normal", "color" }); // ensures #normals >= #positions
    auto normals = mesh->getAttributeBuffer(gpu::Stream::NORMAL);
    auto verts = mesh->getVertexBuffer();
    auto indices = mesh->getIndexBuffer();
    auto esize = indices._element.getSize();
    auto numPoints = indices.getNumElements();
    const auto TRIANGLE = 3;
    quint32 numFaces = (quint32)numPoints / TRIANGLE;
    //QVector<Triangle> faces;
    QVector<glm::vec3> faceNormals;
    QMap<QString,QVector<quint32>> vertexToFaces;
    //faces.resize(numFaces);
    faceNormals.resize(numFaces);
    auto numNormals = normals.getNumElements();
    qCInfo(model_scripting) << QString("numFaces: %1, numNormals: %2, numPoints: %3").arg(numFaces).arg(numNormals).arg(numPoints);
    if (normals.getNumElements() != verts.getNumElements()) {
        return false;
    }
    for (quint32 i = 0; i < numFaces; i++) {
        quint32 I = TRIANGLE * i;
        quint32 i0 = esize == 4 ? indices.get<quint32>(I+0) : indices.get<quint16>(I+0);
        quint32 i1 = esize == 4 ? indices.get<quint32>(I+1) : indices.get<quint16>(I+1);
        quint32 i2 = esize == 4 ? indices.get<quint32>(I+2) : indices.get<quint16>(I+2);

        Triangle face = {
            verts.get<glm::vec3>(i1),
            verts.get<glm::vec3>(i2),
            verts.get<glm::vec3>(i0)
        };
        faceNormals[i] = face.getNormal();
        if (glm::isnan(faceNormals[i].x)) {
            qCInfo(model_scripting) << i << i0 << i1 << i2 << vec3toVariant(face.v0) << vec3toVariant(face.v1) << vec3toVariant(face.v2);
            break;
        }
        vertexToFaces[glm::to_string(face.v0).c_str()] << i;
        vertexToFaces[glm::to_string(face.v1).c_str()] << i;
        vertexToFaces[glm::to_string(face.v2).c_str()] << i;
    }
    for (quint32 j = 0; j < numNormals; j++) {
        //auto v = verts.get<glm::vec3>(j);
        glm::vec3 normal { 0.0f, 0.0f, 0.0f };
        QString key { glm::to_string(verts.get<glm::vec3>(j)).c_str() };
        const auto& faces = vertexToFaces.value(key);
        if (faces.size()) {
            for (const auto i : faces) {
                normal += faceNormals[i];
            }
            normal *= 1.0f / (float)faces.size();
        } else {
            static int logged = 0;
            if (logged++ < 10) {
                qCInfo(model_scripting) << "no faces for key!?" << key;
            }
            normal = verts.get<glm::vec3>(j);
        }
        if (glm::isnan(normal.x)) {
            static int logged = 0;
            if (logged++ < 10) {
                qCInfo(model_scripting) << "isnan(normal.x)" << j << vec3toVariant(normal);
            }
            break;
        }
        normals.edit<glm::vec3>(j) = glm::normalize(normal);
    }
    return true;
}

QScriptValue ModelScriptingInterface::mapMeshAttributeValues(
    scriptable::ScriptableMeshPointer meshProxy, QScriptValue scopeOrCallback, QScriptValue methodOrName
) {
    auto mesh = getMeshPointer(meshProxy);
    if (!mesh) {
        return false;
    }
    auto scopedHandler = makeScopedHandlerObject(scopeOrCallback, methodOrName);

    // input buffers
    gpu::BufferView positions = mesh->getVertexBuffer();

    const auto nPositions = positions.getNumElements();

    // destructure so we can still invoke callback scoped, but with a custom signature (obj, i, jsMesh)
    auto scope = scopedHandler.property("scope");
    auto callback = scopedHandler.property("callback");
    auto js = engine(); // cache value to avoid resolving each iteration
    auto meshPart = js->toScriptValue(meshProxy);

    auto obj = js->newObject();
    auto attributeViews = ScriptableMesh::gatherBufferViews(mesh, { "normal", "color" });
    for (uint32_t i=0; i < nPositions; i++) {
        for (const auto& a : attributeViews) {
            bool asArray = a.second._element.getType() != gpu::FLOAT;
            obj.setProperty(a.first, bufferViewElementToScriptValue(js, a.second, i, asArray, a.first.toStdString().c_str()));
        }
        auto result = callback.call(scope, { obj, i, meshPart });
        if (js->hasUncaughtException()) {
            context()->throwValue(js->uncaughtException());
            return false;
        }

        if (result.isBool() && !result.toBool()) {
            // bail without modifying data if user explicitly returns false
            continue;
        }
        if (result.isObject() && !result.strictlyEquals(obj)) {
            // user returned a new object (ie: instead of modifying input properties)
            obj = result;
        }

        for (const auto& a : attributeViews) {
            const auto& attribute = obj.property(a.first);
            auto& view = a.second;
            if (attribute.isValid()) {
                bufferViewElementFromScriptValue(attribute, view, i);
            }
        }
    }
    return thisObject();
}

void ModelScriptingInterface::getMeshes(QUuid uuid, QScriptValue scopeOrCallback, QScriptValue methodOrName) {
    auto handler = makeScopedHandlerObject(scopeOrCallback, methodOrName);
    Q_ASSERT(handler.engine() == this->engine());
    QPointer<BaseScriptEngine> engine = dynamic_cast<BaseScriptEngine*>(handler.engine());

    scriptable::ScriptableModel meshes;
    bool success = false;
    QString error;

    auto appProvider = DependencyManager::get<scriptable::ModelProviderFactory>();
    qDebug() << "appProvider" << appProvider.data();
    scriptable::ModelProviderPointer provider = appProvider ? appProvider->lookupModelProvider(uuid) : nullptr;
    QString providerType = provider ? provider->metadata.value("providerType").toString() : QString();
    if (providerType.isEmpty()) {
        providerType = "unknown";
    }
    if (provider) {
        qCDebug(model_scripting) << "fetching meshes from " << providerType << "...";
        auto scriptableMeshes = provider->getScriptableModel(&success);
        qCDebug(model_scripting) << "//fetched meshes from " << providerType << "success:" <<success << "#" << scriptableMeshes.meshes.size();
        if (success) {
            meshes = scriptableMeshes;//SimpleModelProxy::fromScriptableModel(scriptableMeshes);
        }
    }
    if (!success) {
        error = QString("failed to get meshes from %1 provider for uuid %2").arg(providerType).arg(uuid.toString());
    }

    if (!error.isEmpty()) {
        qCWarning(model_scripting) << "ModelScriptingInterface::getMeshes ERROR" << error;
        callScopedHandlerObject(handler, engine->makeError(error), QScriptValue::NullValue);
    } else {
        callScopedHandlerObject(handler, QScriptValue::NullValue, engine->toScriptValue(meshes));
    }
}

namespace {
    QScriptValue meshToScriptValue(QScriptEngine* engine, scriptable::ScriptableMeshPointer const &in) {
        return engine->newQObject(in.get(), QScriptEngine::QtOwnership,
            QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects
        );
    }

    void meshFromScriptValue(const QScriptValue& value, scriptable::ScriptableMeshPointer &out) {
        auto obj = value.toQObject();
        //qDebug() << "meshFromScriptValue" << obj;
        if (auto tmp = qobject_cast<scriptable::ScriptableMesh*>(obj)) {
            out = tmp->shared_from_this();
        }
        // FIXME: Why does above cast not work on Win32!?
        if (!out) {
            auto smp = static_cast<scriptable::ScriptableMesh*>(obj);
            //qDebug() << "meshFromScriptValue2" << smp;
            out = smp->shared_from_this();
        }
    }

    QScriptValue meshesToScriptValue(QScriptEngine* engine, const scriptable::ScriptableModelPointer &in) {
        // QScriptValueList result;
        QScriptValue result = engine->newArray();
        int i = 0;
        foreach(scriptable::ScriptableMeshPointer const meshProxy, in->getMeshes()) {
            result.setProperty(i++, meshToScriptValue(engine, meshProxy));
        }
        return result;
    }

    void meshesFromScriptValue(const QScriptValue& value, scriptable::ScriptableModelPointer &out) {
        const auto length = value.property("length").toInt32();
        qCDebug(model_scripting) << "in meshesFromScriptValue, length =" << length;
        for (int i = 0; i < length; i++) {
            if (const auto meshProxy = qobject_cast<scriptable::ScriptableMesh*>(value.property(i).toQObject())) {
                out->meshes.append(meshProxy->getMeshPointer());
            } else {
                qCDebug(model_scripting) << "null meshProxy" << i;
            }
        }
    }

    void modelProxyFromScriptValue(const QScriptValue& object, scriptable::ScriptableModel &meshes) {
        auto meshesProperty = object.property("meshes");
        if (meshesProperty.property("length").toInt32() > 0) {
            //meshes._meshes = qobject_cast<scriptable::ScriptableModelPointer>(meshesProperty.toQObject());
            // qDebug() << "modelProxyFromScriptValue" << meshesProperty.property("length").toInt32() << meshesProperty.toVariant().typeName();
            qScriptValueToSequence(meshesProperty, meshes.meshes);
        } else if (auto mesh = qobject_cast<scriptable::ScriptableMesh*>(object.toQObject())) {
            meshes.meshes << mesh->getMeshPointer();
        } else {
            qDebug() << "modelProxyFromScriptValue -- unrecognized input" << object.toVariant().toString();
        }

        meshes.metadata = object.property("metadata").toVariant().toMap();
    }

    QScriptValue modelProxyToScriptValue(QScriptEngine* engine, const scriptable::ScriptableModel &in) {
        QScriptValue obj = engine->newObject();
        obj.setProperty("meshes", qScriptValueFromSequence(engine, in.meshes));
        obj.setProperty("metadata", engine->toScriptValue(in.metadata));
        return obj;
    }

    QScriptValue meshFaceToScriptValue(QScriptEngine* engine, const mesh::MeshFace &meshFace) {
        QScriptValue obj = engine->newObject();
        obj.setProperty("vertices", qVectorIntToScriptValue(engine, meshFace.vertexIndices));
        return obj;
    }

    void meshFaceFromScriptValue(const QScriptValue &object, mesh::MeshFace& meshFaceResult) {
        qScriptValueToSequence(object.property("vertices"), meshFaceResult.vertexIndices);
    }

    QScriptValue qVectorMeshFaceToScriptValue(QScriptEngine* engine, const QVector<mesh::MeshFace>& vector) {
        return qScriptValueFromSequence(engine, vector);
    }

    void qVectorMeshFaceFromScriptValue(const QScriptValue& array, QVector<mesh::MeshFace>& result) {
        qScriptValueToSequence(array, result);
    }

    QScriptValue qVectorUInt32ToScriptValue(QScriptEngine* engine, const QVector<mesh::uint32>& vector) {
        return qScriptValueFromSequence(engine, vector);
    }

    void qVectorUInt32FromScriptValue(const QScriptValue& array, QVector<mesh::uint32>& result) {
        qScriptValueToSequence(array, result);
    }
}

int meshUint32 = qRegisterMetaType<mesh::uint32>();
namespace mesh {
    int meshUint32 = qRegisterMetaType<uint32>();
}
int qVectorMeshUint32 = qRegisterMetaType<QVector<mesh::uint32>>();

void ModelScriptingInterface::registerMetaTypes(QScriptEngine* engine) {
    qScriptRegisterSequenceMetaType<QVector<scriptable::ScriptableMeshPointer>>(engine);
    qScriptRegisterSequenceMetaType<mesh::MeshFaces>(engine);
    qScriptRegisterSequenceMetaType<QVector<mesh::uint32>>(engine);
    qScriptRegisterMetaType(engine, modelProxyToScriptValue, modelProxyFromScriptValue);

    qScriptRegisterMetaType(engine, qVectorUInt32ToScriptValue, qVectorUInt32FromScriptValue);
    qScriptRegisterMetaType(engine, meshToScriptValue, meshFromScriptValue);
    qScriptRegisterMetaType(engine, meshesToScriptValue, meshesFromScriptValue);
    qScriptRegisterMetaType(engine, meshFaceToScriptValue, meshFaceFromScriptValue);
    qScriptRegisterMetaType(engine, qVectorMeshFaceToScriptValue, qVectorMeshFaceFromScriptValue);
}

MeshPointer ModelScriptingInterface::getMeshPointer(scriptable::ScriptableMeshPointer meshProxy) {
    MeshPointer result;
    if (!meshProxy) {
        if (context()){
            context()->throwError("expected meshProxy as first parameter");
        }
        return result;
    }
    auto mesh = meshProxy->getMeshPointer();
    if (!mesh) {
        if (context()) {
            context()->throwError("expected valid meshProxy as first parameter");
        }
        return result;
    }
    return mesh;
}
