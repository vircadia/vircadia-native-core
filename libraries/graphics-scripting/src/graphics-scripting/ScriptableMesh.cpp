//
//  SimpleMeshProxy.cpp
//  libraries/model-networking/src/model-networking/
//
//  Created by Seth Alves on 2017-3-22.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptableMesh.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>
#include <graphics/Geometry.h>
#include <graphics-scripting/DebugNames.h>
#include <graphics-scripting/BufferViewHelpers.h>
#include <Extents.h>

#include "ScriptableMesh.moc"

#include <RegisteredMetaTypes.h>

QLoggingCategory mesh_logging { "hifi.scripting.mesh" };

// FIXME: unroll/resolve before PR
using namespace scriptable;
QMap<QString,int> ScriptableMesh::ATTRIBUTES{
    {"position", gpu::Stream::POSITION },
    {"normal", gpu::Stream::NORMAL },
    {"color", gpu::Stream::COLOR },
    {"tangent", gpu::Stream::TEXCOORD0 },
    {"skin_cluster_index", gpu::Stream::SKIN_CLUSTER_INDEX },
    {"skin_cluster_weight", gpu::Stream::SKIN_CLUSTER_WEIGHT },
    {"texcoord0", gpu::Stream::TEXCOORD0 },
    {"texcoord1", gpu::Stream::TEXCOORD1 },
    {"texcoord2", gpu::Stream::TEXCOORD2 },
    {"texcoord3", gpu::Stream::TEXCOORD3 },
    {"texcoord4", gpu::Stream::TEXCOORD4 },
};

QVector<scriptable::ScriptableMeshPointer> scriptable::ScriptableModel::getMeshes() const {
    QVector<scriptable::ScriptableMeshPointer> out;
    for(auto& mesh : meshes) {
        out << scriptable::ScriptableMeshPointer(new ScriptableMesh(std::const_pointer_cast<scriptable::ScriptableModel>(this->shared_from_this()), mesh));
    }
    return out;
}

quint32 ScriptableMesh::getNumVertices() const {
    if (auto mesh = getMeshPointer()) {
        return (quint32)mesh->getNumVertices();
    }
    return 0;
}

// glm::vec3 ScriptableMesh::getPos3(quint32 index) const {
//     if (auto mesh = getMeshPointer()) {
//         if (index < getNumVertices()) {
//             return mesh->getPos3(index);
//         }
//     }
//     return glm::vec3(NAN);
// }

namespace {
    gpu::BufferView getBufferView(scriptable::MeshPointer mesh, gpu::Stream::Slot slot) {
        return slot == gpu::Stream::POSITION ? mesh->getVertexBuffer() : mesh->getAttributeBuffer(slot);
    }
}

QVector<quint32> ScriptableMesh::findNearbyIndices(const glm::vec3& origin, float epsilon) const {
    QVector<quint32> result;
    if (auto mesh = getMeshPointer()) {
        const auto& pos = getBufferView(mesh, gpu::Stream::POSITION);
        const uint32_t num = (uint32_t)pos.getNumElements();
        for (uint32_t i = 0; i < num; i++) {
            const auto& position = pos.get<glm::vec3>(i);
            if (glm::distance(position, origin) <= epsilon) {
                result << i;
            }
        }
    }
    return result;
}

QVector<quint32> ScriptableMesh::getIndices() const {
    QVector<quint32> result;
    if (auto mesh = getMeshPointer()) {
        qCDebug(mesh_logging, "getTriangleIndices mesh %p", mesh.get());
        gpu::BufferView indexBufferView = mesh->getIndexBuffer();
        if (quint32 count = (quint32)indexBufferView.getNumElements()) {
            result.resize(count);
            auto buffer = indexBufferView._buffer;
            if (indexBufferView._element.getSize() == 4) {
                // memcpy(result.data(), buffer->getData(), result.size()*sizeof(quint32));
                for (quint32 i = 0; i < count; i++) {
                    result[i] = indexBufferView.get<quint32>(i);
                }
            } else {
                for (quint32 i = 0; i < count; i++) {
                    result[i] = indexBufferView.get<quint16>(i);
                }
            }
        }
    }
    return result;
}

quint32 ScriptableMesh::getNumAttributes() const {
    if (auto mesh = getMeshPointer()) {
        return (quint32)mesh->getNumAttributes();
    }
    return 0;
}
QVector<QString> ScriptableMesh::getAttributeNames() const {
    QVector<QString> result;
    if (auto mesh = getMeshPointer()) {
        for (const auto& a : ATTRIBUTES.toStdMap()) {
            auto bufferView = getBufferView(mesh, a.second);
            if (bufferView.getNumElements() > 0) {
                result << a.first;
            }
        }
    }
    return result;
}

// override
QVariantMap ScriptableMesh::getVertexAttributes(quint32 vertexIndex) const {
    return getVertexAttributes(vertexIndex, getAttributeNames());
}

bool ScriptableMesh::setVertexAttributes(quint32 vertexIndex, QVariantMap attributes) {
    qDebug()  << "setVertexAttributes" << vertexIndex << attributes;
    for (auto& a : gatherBufferViews(getMeshPointer())) {
        const auto& name = a.first;
        const auto& value = attributes.value(name);
        if (value.isValid()) {
            auto& view = a.second;
            bufferViewElementFromVariant(view, vertexIndex, value);
        } else {
            qCDebug(mesh_logging) << "setVertexAttributes" << vertexIndex << name;
        }
    }
    return true;
}

int ScriptableMesh::_getSlotNumber(const QString& attributeName) const {
    if (auto mesh = getMeshPointer()) {
        return ATTRIBUTES.value(attributeName, -1);
    }
    return -1;
}


QVariantMap ScriptableMesh::getMeshExtents() const {
    auto mesh = getMeshPointer();
    auto box = mesh ? mesh->evalPartsBound(0, (int)mesh->getNumParts()) : AABox();
    return {
        { "brn", glmVecToVariant(box.getCorner()) },
        { "tfl", glmVecToVariant(box.calcTopFarLeft()) },
        { "center", glmVecToVariant(box.calcCenter()) },
        { "min", glmVecToVariant(box.getMinimumPoint()) },
        { "max", glmVecToVariant(box.getMaximumPoint()) },
        { "dimensions", glmVecToVariant(box.getDimensions()) },
    };
}

quint32 ScriptableMesh::getNumParts() const {
    if (auto mesh = getMeshPointer()) {
        return (quint32)mesh->getNumParts();
    }
    return 0;
}

QVariantMap ScriptableMesh::scaleToFit(float unitScale) {
    if (auto mesh = getMeshPointer()) {
        auto box = mesh->evalPartsBound(0, (int)mesh->getNumParts());
        auto center = box.calcCenter();
        float maxDimension = glm::distance(box.getMaximumPoint(), box.getMinimumPoint());
        return scale(glm::vec3(unitScale / maxDimension), center);
    }
    return {};
}
QVariantMap ScriptableMesh::translate(const glm::vec3& translation) {
    return transform(glm::translate(translation));
}
QVariantMap ScriptableMesh::scale(const glm::vec3& scale, const glm::vec3& origin) {
    if (auto mesh = getMeshPointer()) {
        auto box = mesh->evalPartsBound(0, (int)mesh->getNumParts());
        glm::vec3 center = glm::isnan(origin.x) ? box.calcCenter() : origin;
        return transform(glm::translate(center) * glm::scale(scale));
    }
    return {};
}
QVariantMap ScriptableMesh::rotateDegrees(const glm::vec3& eulerAngles, const glm::vec3& origin) {
    return rotate(glm::quat(glm::radians(eulerAngles)), origin);
}
QVariantMap ScriptableMesh::rotate(const glm::quat& rotation, const glm::vec3& origin) {
    if (auto mesh = getMeshPointer()) {
        auto box = mesh->evalPartsBound(0, (int)mesh->getNumParts());
        glm::vec3 center = glm::isnan(origin.x) ? box.calcCenter() : origin;
        return transform(glm::translate(center) * glm::toMat4(rotation));
    }
    return {};
}
QVariantMap ScriptableMesh::transform(const glm::mat4& transform) {
    if (auto mesh = getMeshPointer()) {
        const auto& pos = getBufferView(mesh, gpu::Stream::POSITION);
        const uint32_t num = (uint32_t)pos.getNumElements();
        for (uint32_t i = 0; i < num; i++) {
            auto& position = pos.edit<glm::vec3>(i);
            position = transform * glm::vec4(position, 1.0f);
        }
    }
    return getMeshExtents();
}

QVariantList ScriptableMesh::getAttributeValues(const QString& attributeName) const {
    QVariantList result;
    auto slotNum = _getSlotNumber(attributeName);
    if (slotNum >= 0) {
        auto slot = (gpu::Stream::Slot)slotNum;
        const auto& bufferView = getBufferView(getMeshPointer(), slot);
        if (auto len = bufferView.getNumElements()) {
            bool asArray = bufferView._element.getType() != gpu::FLOAT;
            for (quint32 i = 0; i < len; i++) {
                result << bufferViewElementToVariant(bufferView, i, asArray, attributeName.toStdString().c_str());
            }
        }
    }
    return result;
}
QVariantMap ScriptableMesh::getVertexAttributes(quint32 vertexIndex, QVector<QString> names) const {
    QVariantMap result;
    auto mesh = getMeshPointer();
    if (!mesh || vertexIndex >= getNumVertices()) {
        return result;
    }
    for (const auto& a : ATTRIBUTES.toStdMap()) {
        auto name = a.first;
        if (!names.contains(name)) {
            continue;
        }
        auto slot = a.second;
        const gpu::BufferView& bufferView = getBufferView(mesh, slot);
        if (vertexIndex < bufferView.getNumElements()) {
            bool asArray = bufferView._element.getType() != gpu::FLOAT;
            result[name] = bufferViewElementToVariant(bufferView, vertexIndex, asArray, name.toStdString().c_str());
        }
    }
    return result;
}

/// --- buffer view <-> variant helpers

namespace {
    // expand the corresponding attribute buffer (creating it if needed) so that it matches POSITIONS size and specified element type
    gpu::BufferView _expandedAttributeBuffer(const scriptable::MeshPointer mesh, gpu::Stream::Slot slot, const gpu::Element& elementType) {
        gpu::Size elementSize = elementType.getSize();
        gpu::BufferView bufferView = getBufferView(mesh, slot);
        auto nPositions = mesh->getNumVertices();
        auto vsize = nPositions * elementSize;
        auto diffTypes = (elementType.getType() != bufferView._element.getType()  ||
                          elementType.getSize() > bufferView._element.getSize() ||
                          elementType.getScalarCount() > bufferView._element.getScalarCount() ||
                          vsize > bufferView._size
            );
        auto hint = DebugNames::stringFrom(slot);

#ifdef DEV_BUILD
        auto beforeCount = bufferView.getNumElements();
        auto beforeTotal = bufferView._size;
#endif
        if (bufferView.getNumElements() < nPositions || diffTypes) {
            if (!bufferView._buffer || bufferView.getNumElements() == 0) {
                qCInfo(mesh_logging).nospace() << "ScriptableMesh -- adding missing mesh attribute '" << hint << "' for BufferView";
                gpu::Byte *data = new gpu::Byte[vsize];
                memset(data, 0, vsize);
                auto buffer = new gpu::Buffer(vsize, (gpu::Byte*)data);
                delete[] data;
                bufferView = gpu::BufferView(buffer, elementType);
                mesh->addAttribute(slot, bufferView);
            } else {
                qCInfo(mesh_logging) << "ScriptableMesh -- resizing Buffer current:" << hint << bufferView._buffer->getSize() << "wanted:" << vsize;
                bufferView._element = elementType;
                bufferView._buffer->resize(vsize);
                bufferView._size = bufferView._buffer->getSize();
            }
        }
#ifdef DEV_BUILD
        auto afterCount = bufferView.getNumElements();
        auto afterTotal = bufferView._size;
        if (beforeTotal != afterTotal || beforeCount != afterCount) {
            auto typeName = DebugNames::stringFrom(bufferView._element.getType());
            qCDebug(mesh_logging, "NOTE:: _expandedAttributeBuffer.%s vec%d %s (before count=%lu bytes=%lu // after count=%lu bytes=%lu)",
                          hint.toStdString().c_str(), bufferView._element.getScalarCount(),
                          typeName.toStdString().c_str(), beforeCount, beforeTotal, afterCount, afterTotal);
        }
#endif
        return bufferView;
    }
    const gpu::Element UNUSED{ gpu::SCALAR, gpu::UINT8, gpu::RAW };

    gpu::Element getVecNElement(gpu::Type T, int N) {
        switch(N) {
        case 2: return { gpu::VEC2, T, gpu::XY };
        case 3: return { gpu::VEC3, T, gpu::XYZ };
        case 4: return { gpu::VEC4, T, gpu::XYZW };
        }
        Q_ASSERT(false);
        return UNUSED;
    }

    gpu::BufferView expandAttributeToMatchPositions(scriptable::MeshPointer mesh, gpu::Stream::Slot slot) {
        if (slot == gpu::Stream::POSITION) {
            return getBufferView(mesh, slot);
        }
        return _expandedAttributeBuffer(mesh, slot, getVecNElement(gpu::FLOAT, 3));
    }
}

std::map<QString, gpu::BufferView> ScriptableMesh::gatherBufferViews(scriptable::MeshPointer mesh, const QStringList& expandToMatchPositions) {
    std::map<QString, gpu::BufferView> attributeViews;
    if (!mesh) {
        return attributeViews;
    }
    for (const auto& a : ScriptableMesh::ATTRIBUTES.toStdMap()) {
        auto name = a.first;
        auto slot = a.second;
        if (expandToMatchPositions.contains(name)) {
            expandAttributeToMatchPositions(mesh, slot);
        }
        auto view = getBufferView(mesh, slot);
        auto beforeCount = view.getNumElements();
        if (beforeCount > 0) {
            auto element = view._element;
            auto vecN = element.getScalarCount();
            auto type = element.getType();
            QString typeName = DebugNames::stringFrom(element.getType());
            auto beforeTotal = view._size;

            attributeViews[name] = _expandedAttributeBuffer(mesh, slot, getVecNElement(type, vecN));

#if DEV_BUILD
            auto afterTotal = attributeViews[name]._size;
            auto afterCount = attributeViews[name].getNumElements();
            if (beforeTotal != afterTotal || beforeCount != afterCount) {
                qCDebug(mesh_logging, "NOTE:: gatherBufferViews.%s vec%d %s (before count=%lu bytes=%lu // after count=%lu bytes=%lu)",
                              name.toStdString().c_str(), vecN, typeName.toStdString().c_str(), beforeCount, beforeTotal, afterCount, afterTotal);
            }
#endif
        }
    }
    return attributeViews;
}
