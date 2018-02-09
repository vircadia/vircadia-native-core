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
#include <glm/gtx/string_cast.hpp>
#include <graphics/Geometry.h>
#include <graphics-scripting/DebugNames.h>
#include <graphics-scripting/BufferViewHelpers.h>
#include <graphics-scripting/BufferViewScripting.h>
#include <Extents.h>

#include "ScriptableMesh.moc"

#include <RegisteredMetaTypes.h>
#include <BaseScriptEngine.h>
#include <QtScript/QScriptValue>

#include "OBJWriter.h"

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


QString scriptable::ScriptableModel::toString() const {
    return QString("[ScriptableModel%1%2]")
        .arg(objectID.isNull() ? "" : " objectID="+objectID.toString())
        .arg(objectName().isEmpty() ? "" : " name=" +objectName());
}

const QVector<scriptable::ScriptableMeshPointer> scriptable::ScriptableModel::getConstMeshes() const {
    QVector<scriptable::ScriptableMeshPointer> out;
    for(const auto& mesh : meshes) {
        const scriptable::ScriptableMeshPointer m = scriptable::ScriptableMeshPointer(new scriptable::ScriptableMesh(const_cast<scriptable::ScriptableModel*>(this), mesh));
        out << m;
    }
    return out;
}
QVector<scriptable::ScriptableMeshPointer> scriptable::ScriptableModel::getMeshes() {
    QVector<scriptable::ScriptableMeshPointer> out;
    for(auto& mesh : meshes) {
        scriptable::ScriptableMeshPointer m{new scriptable::ScriptableMesh(this, mesh)};
        out << m;
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
    //qDebug()  << "setVertexAttributes" << vertexIndex << attributes;
    for (auto& a : gatherBufferViews(getMeshPointer())) {
        const auto& name = a.first;
        const auto& value = attributes.value(name);
        if (value.isValid()) {
            auto& view = a.second;
            //qCDebug(mesh_logging) << "setVertexAttributes" << vertexIndex << name;
            bufferViewElementFromVariant(view, vertexIndex, value);
        } else {
            //qCDebug(mesh_logging) << "(skipping) setVertexAttributes" << vertexIndex << name;
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

QScriptValue ScriptableModel::mapAttributeValues(QScriptValue scopeOrCallback, QScriptValue methodOrName) {
    auto context = scopeOrCallback.engine()->currentContext();
    auto _in = context->thisObject();
    qCInfo(mesh_logging) << "mapAttributeValues" << _in.toVariant().typeName() << _in.toVariant().toString() << _in.toQObject();
    auto model = qscriptvalue_cast<scriptable::ScriptableModel>(_in);
    QVector<scriptable::ScriptableMeshPointer> in = model.getMeshes();
    if (in.size()) {
        foreach (scriptable::ScriptableMeshPointer meshProxy, in) {
            meshProxy->mapAttributeValues(scopeOrCallback, methodOrName);
        }
        return _in;
    } else if (auto meshProxy = qobject_cast<scriptable::ScriptableMesh*>(_in.toQObject())) {
        return meshProxy->mapAttributeValues(scopeOrCallback, methodOrName);
    } else {
        context->throwError("invalid ModelProxy || MeshProxyPointer");
    }
    return false;
}



QScriptValue ScriptableMesh::mapAttributeValues(QScriptValue scopeOrCallback, QScriptValue methodOrName) {
    auto mesh = getMeshPointer();
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
    auto meshPart = thisObject();//js->toScriptValue(meshProxy);

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

QScriptValue ScriptableMesh::unrollVertices(bool recalcNormals) {
    auto meshProxy = this;
    auto mesh = getMeshPointer();
    qCInfo(mesh_logging) << "ScriptableMesh::unrollVertices" << !!mesh<< !!meshProxy;
    if (!mesh) {
        return QScriptValue();
    }

    auto positions = mesh->getVertexBuffer();
    auto indices = mesh->getIndexBuffer();
    quint32 numPoints = (quint32)indices.getNumElements();
    auto buffer = new gpu::Buffer();
    buffer->resize(numPoints * sizeof(uint32_t));
    auto newindices = gpu::BufferView(buffer, { gpu::SCALAR, gpu::UINT32, gpu::INDEX });
    qCInfo(mesh_logging) << "ScriptableMesh::unrollVertices numPoints" << numPoints;
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
        qCInfo(mesh_logging) << "ScriptableMesh::unrollVertices buffer" << a.first;
        qCInfo(mesh_logging) << "ScriptableMesh::unrollVertices source" << view.getNumElements();
        qCInfo(mesh_logging) << "ScriptableMesh::unrollVertices dest" << points.getNumElements();
        qCInfo(mesh_logging) << "ScriptableMesh::unrollVertices sz" << sz << src << dest << slot;
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
        recalculateNormals();
    }
    return true;
}

bool ScriptableMesh::replaceMeshData(scriptable::ScriptableMeshPointer src, const QVector<QString>& attributeNames) {
    auto target = getMeshPointer();
    auto source = src ? src->getMeshPointer() : nullptr;
    if (!target || !source) {
        context()->throwError("ScriptableMesh::replaceMeshData -- expected dest and src to be valid mesh proxy pointers");
        return false;
    }

    QVector<QString> attributes = attributeNames.isEmpty() ? src->getAttributeNames() : attributeNames;

    //qCInfo(mesh_logging) << "ScriptableMesh::replaceMeshData -- source:" << source->displayName << "target:" << target->displayName << "attributes:" << attributes;

    // remove attributes only found on target mesh, unless user has explicitly specified the relevant attribute names
    if (attributeNames.isEmpty()) {
        auto attributeViews = ScriptableMesh::gatherBufferViews(target);
        for (const auto& a : attributeViews) {
            auto slot = ScriptableMesh::ATTRIBUTES[a.first];
            if (!attributes.contains(a.first)) {
                //qCInfo(mesh_logging) << "ScriptableMesh::replaceMeshData -- pruning target attribute" << a.first << slot;
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
            //qCInfo(mesh_logging) << "ScriptableMesh::replaceMeshData buffer is empty -- pruning" << a << slot;
            target->removeAttribute(slot);
        } else {
            // if (before.getNumElements() == 0) {
            //     qCInfo(mesh_logging) << "ScriptableMesh::replaceMeshData target buffer is empty -- adding" << a << slot;
            // } else {
            //     qCInfo(mesh_logging) << "ScriptableMesh::replaceMeshData target buffer exists -- updating" << a << slot;
            // }
            target->addAttribute(slot, cloneBufferView(input));
        }
        // auto& after = target->getAttributeBuffer(slot);
        // qCInfo(mesh_logging) << "ScriptableMesh::replaceMeshData" << a << slot << before.getNumElements() << " -> " << after.getNumElements();
    }


    return true;
}

bool ScriptableMesh::dedupeVertices(float epsilon) {
    scriptable::ScriptableMeshPointer meshProxy = this;
    auto mesh = getMeshPointer();
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

    qCInfo(mesh_logging) << "//VERTS before" << numPositions << "after" << uniqueVerts.size();

    auto indices = mesh->getIndexBuffer();
    auto numIndices = indices.getNumElements();
    auto esize = indices._element.getSize();
    QVector<quint32> newIndices;
    newIndices.reserve((int)numIndices);
    for (quint32 i = 0; i < numIndices; i++) {
        quint32 index = esize == 4 ? indices.get<quint32>(i) : indices.get<quint16>(i);
        if (remapIndices.contains(index)) {
            //qCInfo(mesh_logging) << i << index << "->" << remapIndices[index];
            newIndices << remapIndices[index];
        } else {
            qCInfo(mesh_logging) << i << index << "!remapIndices[index]";
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
        qCInfo(mesh_logging) << "ScriptableMesh::dedupeVertices" << a.first << slot << view.getNumElements();
        auto newView = resizedBufferView(view, numUniqueVerts);
        qCInfo(mesh_logging) << a.first << "before: #" << view.getNumElements() << "after: #" << newView.getNumElements();
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

QScriptValue ScriptableMesh::cloneMesh(bool recalcNormals) {
    auto mesh = getMeshPointer();
    if (!mesh) {
        return QScriptValue::NullValue;
    }
    graphics::MeshPointer clone(new graphics::Mesh());
    clone->displayName = mesh->displayName + "-clone";
    qCInfo(mesh_logging) << "ScriptableMesh::cloneMesh" << !!mesh;
    if (!mesh) {
        return QScriptValue::NullValue;
    }

    clone->setIndexBuffer(cloneBufferView(mesh->getIndexBuffer()));
    clone->setPartBuffer(cloneBufferView(mesh->getPartBuffer()));
    auto attributeViews = ScriptableMesh::gatherBufferViews(mesh);
    for (const auto& a : attributeViews) {
        auto& view = a.second;
        auto slot = ScriptableMesh::ATTRIBUTES[a.first];
        qCInfo(mesh_logging) << "ScriptableMesh::cloneVertices buffer" << a.first << slot;
        auto points = cloneBufferView(view);
        qCInfo(mesh_logging) << "ScriptableMesh::cloneVertices source" << view.getNumElements();
        qCInfo(mesh_logging) << "ScriptableMesh::cloneVertices dest" << points.getNumElements();
        if (slot == gpu::Stream::POSITION) {
            clone->setVertexBuffer(points);
        } else {
            clone->addAttribute(slot, points);
        }
    }

    auto result = scriptable::ScriptableMeshPointer(new ScriptableMesh(nullptr, clone));
    if (recalcNormals) {
        result->recalculateNormals();
    }
    return engine()->toScriptValue(result);
}

bool ScriptableMesh::recalculateNormals() {
    scriptable::ScriptableMeshPointer meshProxy = this;
    qCInfo(mesh_logging) << "Recalculating normals" << !!meshProxy;
    auto mesh = getMeshPointer();
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
    qCInfo(mesh_logging) << QString("numFaces: %1, numNormals: %2, numPoints: %3").arg(numFaces).arg(numNormals).arg(numPoints);
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
            qCInfo(mesh_logging) << i << i0 << i1 << i2 << vec3toVariant(face.v0) << vec3toVariant(face.v1) << vec3toVariant(face.v2);
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
                qCInfo(mesh_logging) << "no faces for key!?" << key;
            }
            normal = verts.get<glm::vec3>(j);
        }
        if (glm::isnan(normal.x)) {
            static int logged = 0;
            if (logged++ < 10) {
                qCInfo(mesh_logging) << "isnan(normal.x)" << j << vec3toVariant(normal);
            }
            break;
        }
        normals.edit<glm::vec3>(j) = glm::normalize(normal);
    }
    return true;
}

QString ScriptableMesh::toOBJ() {
    if (!getMeshPointer()) {
        context()->throwError(QString("null mesh"));
    }
    return writeOBJToString({ getMeshPointer() });
}

