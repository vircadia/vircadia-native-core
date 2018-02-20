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

#include "GraphicsScriptingUtil.h"
#include "ScriptableMesh.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>
#include <graphics/Geometry.h>
#include <graphics-scripting/DebugNames.h>
#include <graphics-scripting/BufferViewHelpers.h>
#include <graphics-scripting/BufferViewScripting.h>

#include "ScriptableMesh.moc"

#include <RegisteredMetaTypes.h>
#include <BaseScriptEngine.h>
#include <QtScript/QScriptValue>

#include "OBJWriter.h"

// #define SCRIPTABLE_MESH_DEBUG

namespace scriptable {
    // QScriptValue jsBindCallback(QScriptValue callback);
    // template <typename T> QPointer<T> qpointer_qobject_cast(const QScriptValue& value);
    // template <typename T> T this_qobject_cast(QScriptEngine* engine);
    // template <typename T, class... Rest> QPointer<T> make_scriptowned(Rest... rest);
}

scriptable::ScriptableMeshPart::ScriptableMeshPart(scriptable::ScriptableMeshPointer parentMesh, int partIndex)
    : parentMesh(parentMesh), partIndex(partIndex)  {
    setObjectName(QString("%1.part[%2]").arg(parentMesh ? parentMesh->objectName() : "").arg(partIndex));
}

scriptable::ScriptableMesh::ScriptableMesh(const ScriptableMeshBase& other)
    : ScriptableMeshBase(other) {
    auto mesh = getMeshPointer();
    QString name = mesh ? QString::fromStdString(mesh->modelName) : "";
    if (name.isEmpty()) {
        name = mesh ? QString::fromStdString(mesh->displayName) : "";
    }
    auto parentModel = getParentModel();
    setObjectName(QString("%1#%2").arg(parentModel ? parentModel->objectName() : "").arg(name));
}

QVector<scriptable::ScriptableMeshPartPointer> scriptable::ScriptableMesh::getMeshParts() const {
    QVector<scriptable::ScriptableMeshPartPointer> out;
    for (quint32 i = 0; i < getNumParts(); i++) {
        out << scriptable::make_scriptowned<scriptable::ScriptableMeshPart>(getSelf(), i);
    }
    return out;
}

quint32 scriptable::ScriptableMesh::getNumIndices() const {
    if (auto mesh = getMeshPointer()) {
        return (quint32)mesh->getNumIndices();
    }
    return 0;
}

quint32 scriptable::ScriptableMesh::getNumVertices() const {
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

QVector<quint32> scriptable::ScriptableMesh::findNearbyIndices(const glm::vec3& origin, float epsilon) const {
    QVector<quint32> result;
    if (auto mesh = getMeshPointer()) {
        const auto& pos = buffer_helpers::getBufferView(mesh, gpu::Stream::POSITION);
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

QVector<quint32> scriptable::ScriptableMesh::getIndices() const {
    QVector<quint32> result;
    if (auto mesh = getMeshPointer()) {
        qCDebug(graphics_scripting, "getTriangleIndices mesh %p", mesh.get());
        gpu::BufferView indexBufferView = mesh->getIndexBuffer();
        if (quint32 count = (quint32)indexBufferView.getNumElements()) {
            result.resize(count);
            switch(indexBufferView._element.getType()) {
            case gpu::UINT32:
                // memcpy(result.data(), buffer->getData(), result.size()*sizeof(quint32));
                for (quint32 i = 0; i < count; i++) {
                    result[i] = indexBufferView.get<quint32>(i);
                }
                break;
            case gpu::UINT16:
                for (quint32 i = 0; i < count; i++) {
                    result[i] = indexBufferView.get<quint16>(i);
                }
                break;
            default:
                assert(false);
                Q_ASSERT(false);
            }
        }
    }
    return result;
}

quint32 scriptable::ScriptableMesh::getNumAttributes() const {
    if (auto mesh = getMeshPointer()) {
        return (quint32)mesh->getNumAttributes();
    }
    return 0;
}
QVector<QString> scriptable::ScriptableMesh::getAttributeNames() const {
    QVector<QString> result;
    if (auto mesh = getMeshPointer()) {
        for (const auto& a : buffer_helpers::ATTRIBUTES.toStdMap()) {
            auto bufferView = buffer_helpers::getBufferView(mesh, a.second);
            if (bufferView.getNumElements() > 0) {
                result << a.first;
            }
        }
    }
    return result;
}

// override
QVariantMap scriptable::ScriptableMesh::getVertexAttributes(quint32 vertexIndex) const {
    return getVertexAttributes(vertexIndex, getAttributeNames());
}

bool scriptable::ScriptableMesh::setVertexAttributes(quint32 vertexIndex, QVariantMap attributes) {
    //qCInfo(graphics_scripting)  << "setVertexAttributes" << vertexIndex << attributes;
    metadata["last-modified"] = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);
    for (auto& a : buffer_helpers::gatherBufferViews(getMeshPointer())) {
        const auto& name = a.first;
        const auto& value = attributes.value(name);
        if (value.isValid()) {
            auto& view = a.second;
            //qCDebug(graphics_scripting) << "setVertexAttributes" << vertexIndex << name;
            buffer_helpers::fromVariant(view, vertexIndex, value);
        } else {
            //qCDebug(graphics_scripting) << "(skipping) setVertexAttributes" << vertexIndex << name;
        }
    }
    return true;
}

int scriptable::ScriptableMesh::_getSlotNumber(const QString& attributeName) const {
    if (auto mesh = getMeshPointer()) {
        return buffer_helpers::ATTRIBUTES.value(attributeName, -1);
    }
    return -1;
}


QVariantMap scriptable::ScriptableMesh::getMeshExtents() const {
    auto mesh = getMeshPointer();
    auto box = mesh ? mesh->evalPartsBound(0, (int)mesh->getNumParts()) : AABox();
    return buffer_helpers::toVariant(box).toMap();
}

quint32 scriptable::ScriptableMesh::getNumParts() const {
    if (auto mesh = getMeshPointer()) {
        return (quint32)mesh->getNumParts();
    }
    return 0;
}

QVariantMap scriptable::ScriptableMeshPart::scaleToFit(float unitScale) {
    if (auto mesh = getMeshPointer()) {
        auto box = mesh->evalPartsBound(0, (int)mesh->getNumParts());
        auto center = box.calcCenter();
        float maxDimension = glm::distance(box.getMaximumPoint(), box.getMinimumPoint());
        return scale(glm::vec3(unitScale / maxDimension), center);
    }
    return {};
}
QVariantMap scriptable::ScriptableMeshPart::translate(const glm::vec3& translation) {
    return transform(glm::translate(translation));
}
QVariantMap scriptable::ScriptableMeshPart::scale(const glm::vec3& scale, const glm::vec3& origin) {
    if (auto mesh = getMeshPointer()) {
        auto box = mesh->evalPartsBound(0, (int)mesh->getNumParts());
        glm::vec3 center = glm::isnan(origin.x) ? box.calcCenter() : origin;
        return transform(glm::translate(center) * glm::scale(scale));
    }
    return {};
}
QVariantMap scriptable::ScriptableMeshPart::rotateDegrees(const glm::vec3& eulerAngles, const glm::vec3& origin) {
    return rotate(glm::quat(glm::radians(eulerAngles)), origin);
}
QVariantMap scriptable::ScriptableMeshPart::rotate(const glm::quat& rotation, const glm::vec3& origin) {
    if (auto mesh = getMeshPointer()) {
        auto box = mesh->evalPartsBound(0, (int)mesh->getNumParts());
        glm::vec3 center = glm::isnan(origin.x) ? box.calcCenter() : origin;
        return transform(glm::translate(center) * glm::toMat4(rotation));
    }
    return {};
}
QVariantMap scriptable::ScriptableMeshPart::transform(const glm::mat4& transform) {
    if (auto mesh = getMeshPointer()) {
        const auto& pos = buffer_helpers::getBufferView(mesh, gpu::Stream::POSITION);
        const uint32_t num = (uint32_t)pos.getNumElements();
        for (uint32_t i = 0; i < num; i++) {
            auto& position = pos.edit<glm::vec3>(i);
            position = transform * glm::vec4(position, 1.0f);
        }
        return parentMesh->getMeshExtents();
    }
    return {};
}

QVariantList scriptable::ScriptableMesh::getAttributeValues(const QString& attributeName) const {
    QVariantList result;
    auto slotNum = _getSlotNumber(attributeName);
    if (slotNum >= 0) {
        auto slot = (gpu::Stream::Slot)slotNum;
        const auto& bufferView = buffer_helpers::getBufferView(getMeshPointer(), slot);
        if (auto len = bufferView.getNumElements()) {
            bool asArray = bufferView._element.getType() != gpu::FLOAT;
            for (quint32 i = 0; i < len; i++) {
                result << buffer_helpers::toVariant(bufferView, i, asArray, attributeName.toStdString().c_str());
            }
        }
    }
    return result;
}
QVariantMap scriptable::ScriptableMesh::getVertexAttributes(quint32 vertexIndex, QVector<QString> names) const {
    QVariantMap result;
    auto mesh = getMeshPointer();
    if (!mesh || vertexIndex >= getNumVertices()) {
        return result;
    }
    for (const auto& a : buffer_helpers::ATTRIBUTES.toStdMap()) {
        auto name = a.first;
        if (!names.contains(name)) {
            continue;
        }
        auto slot = a.second;
        const gpu::BufferView& bufferView = buffer_helpers::getBufferView(mesh, slot);
        if (vertexIndex < bufferView.getNumElements()) {
            bool asArray = bufferView._element.getType() != gpu::FLOAT;
            result[name] = buffer_helpers::toVariant(bufferView, vertexIndex, asArray, name.toStdString().c_str());
        }
    }
    return result;
}

quint32 scriptable::ScriptableMesh::mapAttributeValues(QScriptValue _callback) {
    auto mesh = getMeshPointer();
    if (!mesh) {
        return 0;
    }
    auto scopedHandler = jsBindCallback(_callback);

    // input buffers
    gpu::BufferView positions = mesh->getVertexBuffer();

    const auto nPositions = positions.getNumElements();

    // destructure so we can still invoke callback scoped, but with a custom signature (obj, i, jsMesh)
    auto scope = scopedHandler.property("scope");
    auto callback = scopedHandler.property("callback");
    auto js = engine() ? engine() : scopedHandler.engine(); // cache value to avoid resolving each iteration
    if (!js) {
        return 0;
    }
    auto meshPart = js ? js->toScriptValue(getSelf()) : QScriptValue::NullValue;
#ifdef SCRIPTABLE_MESH_DEBUG
    qCInfo(graphics_scripting) << "mapAttributeValues" << mesh.get() << js->currentContext()->thisObject().toQObject();
#endif
    auto obj = js->newObject();
    auto attributeViews = buffer_helpers::gatherBufferViews(mesh, { "normal", "color" });
    metadata["last-modified"] = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);
    uint32_t i = 0;
    for (; i < nPositions; i++) {
        for (const auto& a : attributeViews) {
            bool asArray = a.second._element.getType() != gpu::FLOAT;
            obj.setProperty(a.first, bufferViewElementToScriptValue(js, a.second, i, asArray, a.first.toStdString().c_str()));
        }
        auto result = callback.call(scope, { obj, i, meshPart });
        if (js->hasUncaughtException()) {
            js->currentContext()->throwValue(js->uncaughtException());
            return i;
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
    return i;
}

quint32 scriptable::ScriptableMeshPart::mapAttributeValues(QScriptValue callback) {
    return parentMesh ? parentMesh->mapAttributeValues(callback) : 0; 
}

bool scriptable::ScriptableMeshPart::unrollVertices(bool recalcNormals) {
    auto mesh = getMeshPointer();
#ifdef SCRIPTABLE_MESH_DEBUG
    qCInfo(graphics_scripting) << "ScriptableMeshPart::unrollVertices" << !!mesh<< !!meshProxy;
#endif
    if (!mesh) {
        return false;
    }

    auto positions = mesh->getVertexBuffer();
    auto indices = mesh->getIndexBuffer();
    quint32 numPoints = (quint32)indices.getNumElements();
    auto buffer = new gpu::Buffer();
    buffer->resize(numPoints * sizeof(uint32_t));
    auto newindices = gpu::BufferView(buffer, { gpu::SCALAR, gpu::UINT32, gpu::INDEX });
    metadata["last-modified"] = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);
    qCInfo(graphics_scripting) << "ScriptableMeshPart::unrollVertices numPoints" << numPoints;
    auto attributeViews = buffer_helpers::gatherBufferViews(mesh);
    for (const auto& a : attributeViews) {
        auto& view = a.second;
        auto sz = view._element.getSize();
        auto buffer = new gpu::Buffer();
        buffer->resize(numPoints * sz);
        auto points = gpu::BufferView(buffer, view._element);
        auto src = (uint8_t*)view._buffer->getData();
        auto dest = (uint8_t*)points._buffer->getData();
        auto slot = buffer_helpers::ATTRIBUTES[a.first];
        if (0) {
            qCInfo(graphics_scripting) << "ScriptableMeshPart::unrollVertices buffer" << a.first;
            qCInfo(graphics_scripting) << "ScriptableMeshPart::unrollVertices source" << view.getNumElements();
            qCInfo(graphics_scripting) << "ScriptableMeshPart::unrollVertices dest" << points.getNumElements();
            qCInfo(graphics_scripting) << "ScriptableMeshPart::unrollVertices sz" << sz << src << dest << slot;
        }
        auto esize = indices._element.getSize();
        const char* hint= a.first.toStdString().c_str();
        for(quint32 i = 0; i < numPoints; i++) {
            quint32 index = esize == 4 ? indices.get<quint32>(i) : indices.get<quint16>(i);
            newindices.edit<uint32_t>(i) = i;
            buffer_helpers::fromVariant(
                points, i,
                buffer_helpers::toVariant(view, index, false, hint)
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

bool scriptable::ScriptableMeshPart::replaceMeshData(scriptable::ScriptableMeshPartPointer src, const QVector<QString>& attributeNames) {
    auto target = getMeshPointer();
    auto source = src ? src->getMeshPointer() : nullptr;
    if (!target || !source) {
        if (context()) {
            context()->throwError("ScriptableMeshPart::replaceMeshData -- expected dest and src to be valid mesh proxy pointers");
        } else {
            qCWarning(graphics_scripting) << "ScriptableMeshPart::replaceMeshData -- expected dest and src to be valid mesh proxy pointers";
        }
        return false;
    }

    QVector<QString> attributes = attributeNames.isEmpty() ? src->parentMesh->getAttributeNames() : attributeNames;

    qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData -- " <<
        "source:" << QString::fromStdString(source->displayName) <<
        "target:" << QString::fromStdString(target->displayName) <<
        "attributes:" << attributes;

    metadata["last-modified"] = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);

    // remove attributes only found on target mesh, unless user has explicitly specified the relevant attribute names
    if (attributeNames.isEmpty()) {
        auto attributeViews = buffer_helpers::gatherBufferViews(target);
        for (const auto& a : attributeViews) {
            auto slot = buffer_helpers::ATTRIBUTES[a.first];
            if (!attributes.contains(a.first)) {
                qCInfo(graphics_scripting) << "ScriptableMesh::replaceMeshData -- pruning target attribute" << a.first << slot;
                target->removeAttribute(slot);
            }
        }
    }

    target->setVertexBuffer(buffer_helpers::clone(source->getVertexBuffer()));
    target->setIndexBuffer(buffer_helpers::clone(source->getIndexBuffer()));
    target->setPartBuffer(buffer_helpers::clone(source->getPartBuffer()));

    for (const auto& a : attributes) {
        auto slot = buffer_helpers::ATTRIBUTES[a];
        if (slot == gpu::Stream::POSITION) {
            continue;
        }
        auto& before = target->getAttributeBuffer(slot);
        auto& input = source->getAttributeBuffer(slot);
        if (input.getNumElements() == 0) {
            qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData buffer is empty -- pruning" << a << slot;
            target->removeAttribute(slot);
        } else {
            if (before.getNumElements() == 0) {
                qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData target buffer is empty -- adding" << a << slot;
            } else {
                qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData target buffer exists -- updating" << a << slot;
            }
            target->addAttribute(slot, buffer_helpers::clone(input));
        }
        auto& after = target->getAttributeBuffer(slot);
        qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData" << a << slot << before.getNumElements() << " -> " << after.getNumElements();
    }


    return true;
}

bool scriptable::ScriptableMeshPart::dedupeVertices(float epsilon) {
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

    metadata["last-modified"] = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);
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

    qCInfo(graphics_scripting) << "//VERTS before" << numPositions << "after" << uniqueVerts.size();

    auto indices = mesh->getIndexBuffer();
    auto numIndices = indices.getNumElements();
    auto esize = indices._element.getSize();
    QVector<quint32> newIndices;
    newIndices.reserve((int)numIndices);
    for (quint32 i = 0; i < numIndices; i++) {
        quint32 index = esize == 4 ? indices.get<quint32>(i) : indices.get<quint16>(i);
        if (remapIndices.contains(index)) {
            //qCInfo(graphics_scripting) << i << index << "->" << remapIndices[index];
            newIndices << remapIndices[index];
        } else {
            qCInfo(graphics_scripting) << i << index << "!remapIndices[index]";
        }
    }

    mesh->setIndexBuffer(buffer_helpers::fromVector(newIndices, { gpu::SCALAR, gpu::UINT32, gpu::INDEX }));
    mesh->setVertexBuffer(buffer_helpers::fromVector(uniqueVerts, { gpu::VEC3, gpu::FLOAT, gpu::XYZ }));

    auto attributeViews = buffer_helpers::gatherBufferViews(mesh);
    quint32 numUniqueVerts = uniqueVerts.size();
    for (const auto& a : attributeViews) {
        auto& view = a.second;
        auto slot = buffer_helpers::ATTRIBUTES[a.first];
        if (slot == gpu::Stream::POSITION) {
            continue;
        }
        qCInfo(graphics_scripting) << "ScriptableMeshPart::dedupeVertices" << a.first << slot << view.getNumElements();
        auto newView = buffer_helpers::resize(view, numUniqueVerts);
        qCInfo(graphics_scripting) << a.first << "before: #" << view.getNumElements() << "after: #" << newView.getNumElements();
        quint32 numElements = (quint32)view.getNumElements();
        for (quint32 i = 0; i < numElements; i++) {
            quint32 fromVertexIndex = i;
            quint32 toVertexIndex = remapIndices.contains(fromVertexIndex) ? remapIndices[fromVertexIndex] : fromVertexIndex;
            buffer_helpers::fromVariant(
                newView, toVertexIndex,
                buffer_helpers::toVariant(view, fromVertexIndex, false, "dedupe")
                );
        }
        mesh->addAttribute(slot, newView);
    }
    return true;
}

scriptable::ScriptableMeshPointer scriptable::ScriptableMesh::cloneMesh(bool recalcNormals) {
    auto mesh = getMeshPointer();
    if (!mesh) {
        qCInfo(graphics_scripting) << "ScriptableMesh::cloneMesh -- !meshPointer";
        return nullptr;
    }
    // qCInfo(graphics_scripting) << "ScriptableMesh::cloneMesh...";
    auto clone = buffer_helpers::cloneMesh(mesh);
    
    // qCInfo(graphics_scripting) << "ScriptableMesh::cloneMesh...";
    if (recalcNormals) {
        buffer_helpers::recalculateNormals(clone);
    }
    //qCDebug(graphics_scripting) << clone.get();// << metadata;
    auto meshPointer = scriptable::make_scriptowned<scriptable::ScriptableMesh>(provider, model, clone, metadata);
    clone.reset(); // free local reference
    // qCInfo(graphics_scripting) << "========= ScriptableMesh::cloneMesh..." << meshPointer << meshPointer->ownedMesh.use_count();
    //scriptable::MeshPointer* ppMesh = new scriptable::MeshPointer();
    //*ppMesh = clone;

    if (0 && meshPointer) {
        scriptable::WeakMeshPointer delme = meshPointer->mesh;
        QString debugString = scriptable::toDebugString(meshPointer);
        QObject::connect(meshPointer, &QObject::destroyed, meshPointer, [=]() {
            // qCWarning(graphics_scripting) << "*************** cloneMesh/Destroy";
            // qCWarning(graphics_scripting) << "*************** " << debugString << delme.lock().get();
            if (!delme.expired()) {
                QTimer::singleShot(250, this, [=]{
                    if (!delme.expired()) {
                        qCWarning(graphics_scripting) << "cloneMesh -- potential memory leak..." << debugString << delme.use_count();
                    }
                });
            }
        });
    }
            
    meshPointer->metadata["last-modified"] = QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC).toString(Qt::ISODate);
    return scriptable::ScriptableMeshPointer(meshPointer);
}

scriptable::ScriptableMeshBase::ScriptableMeshBase(scriptable::WeakModelProviderPointer provider, scriptable::ScriptableModelBasePointer model, scriptable::WeakMeshPointer mesh, const QVariantMap& metadata)
    : provider(provider), model(model), mesh(mesh), metadata(metadata) {}
scriptable::ScriptableMeshBase::ScriptableMeshBase(scriptable::WeakMeshPointer mesh) : scriptable::ScriptableMeshBase(scriptable::WeakModelProviderPointer(), nullptr, mesh, QVariantMap()) { }
scriptable::ScriptableMeshBase::ScriptableMeshBase(scriptable::MeshPointer mesh, const QVariantMap& metadata)
    : ScriptableMeshBase(WeakModelProviderPointer(), nullptr, mesh, metadata) {
    ownedMesh = mesh;
}
//scriptable::ScriptableMeshBase::ScriptableMeshBase(const scriptable::ScriptableMeshBase& other) { *this = other; }
scriptable::ScriptableMeshBase& scriptable::ScriptableMeshBase::operator=(const scriptable::ScriptableMeshBase& view) {
    provider = view.provider;
    model = view.model;
    mesh = view.mesh;
    ownedMesh = view.ownedMesh;
    metadata = view.metadata;
    return *this;
}
                                                                                                                                                                                               scriptable::ScriptableMeshBase::~ScriptableMeshBase() {
    ownedMesh.reset();
#ifdef SCRIPTABLE_MESH_DEBUG
    qCInfo(graphics_scripting) << "//~ScriptableMeshBase" << this << "ownedMesh:"  << ownedMesh.use_count() << "mesh:" << mesh.use_count();
#endif
}

scriptable::ScriptableMesh::~ScriptableMesh() {
    ownedMesh.reset();
#ifdef SCRIPTABLE_MESH_DEBUG
    qCInfo(graphics_scripting) << "//~ScriptableMesh" << this << "ownedMesh:"  << ownedMesh.use_count() << "mesh:" << mesh.use_count();
#endif
}

QString scriptable::ScriptableMeshPart::toOBJ() {
    if (!getMeshPointer()) {
        if (context()) {
            context()->throwError(QString("null mesh"));
        } else {
            qCWarning(graphics_scripting) << "null mesh";
        }            
        return QString();
    }
    return writeOBJToString({ getMeshPointer() });
}

namespace {
    template <typename T>
    QScriptValue qObjectToScriptValue(QScriptEngine* engine, const T& object) {
        if (!object) {
            return QScriptValue::NullValue;
        }
        auto ownership = object->metadata.value("__ownership__");
        return engine->newQObject(
            object,
            ownership.isValid() ? static_cast<QScriptEngine::ValueOwnership>(ownership.toInt()) : QScriptEngine::QtOwnership
            //, QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects
        );
    }

    QScriptValue meshPointerToScriptValue(QScriptEngine* engine, const scriptable::ScriptableMeshPointer& in) {
        return qObjectToScriptValue(engine, in);
    }
    QScriptValue meshPartPointerToScriptValue(QScriptEngine* engine, const scriptable::ScriptableMeshPartPointer& in) {
        return qObjectToScriptValue(engine, in);
    }
    QScriptValue modelPointerToScriptValue(QScriptEngine* engine, const scriptable::ScriptableModelPointer& in) {
        return qObjectToScriptValue(engine, in);
    }

    void meshPointerFromScriptValue(const QScriptValue& value, scriptable::ScriptableMeshPointer &out) {
        out = scriptable::qpointer_qobject_cast<scriptable::ScriptableMesh>(value);
    }
    void modelPointerFromScriptValue(const QScriptValue& value, scriptable::ScriptableModelPointer &out) {
        out = scriptable::qpointer_qobject_cast<scriptable::ScriptableModel>(value);
    }
    void meshPartPointerFromScriptValue(const QScriptValue& value, scriptable::ScriptableMeshPartPointer &out) {
        out = scriptable::qpointer_qobject_cast<scriptable::ScriptableMeshPart>(value);
    }
    
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

    QScriptValue qVectorUInt32ToScriptValue(QScriptEngine* engine, const QVector<scriptable::uint32>& vector) {
        return qScriptValueFromSequence(engine, vector);
    }

    void qVectorUInt32FromScriptValue(const QScriptValue& array, QVector<scriptable::uint32>& result) {
        qScriptValueToSequence(array, result);
    }

    QVector<int> metaTypeIds{
        qRegisterMetaType<scriptable::uint32>("uint32"),
        qRegisterMetaType<scriptable::uint32>("scriptable::uint32"),
        qRegisterMetaType<QVector<scriptable::uint32>>(),
        qRegisterMetaType<QVector<scriptable::uint32>>("QVector<uint32>"),
        qRegisterMetaType<scriptable::ScriptableMeshPointer>(),
        qRegisterMetaType<scriptable::ScriptableModelPointer>(),
        qRegisterMetaType<scriptable::ScriptableMeshPartPointer>(),
    };
}

namespace scriptable {
    bool registerMetaTypes(QScriptEngine* engine) {
        qScriptRegisterSequenceMetaType<QVector<scriptable::ScriptableMeshPartPointer>>(engine);
        qScriptRegisterSequenceMetaType<QVector<scriptable::ScriptableMeshPointer>>(engine);
        qScriptRegisterSequenceMetaType<QVector<scriptable::uint32>>(engine);
        
        qScriptRegisterMetaType(engine, qVectorUInt32ToScriptValue, qVectorUInt32FromScriptValue);
        qScriptRegisterMetaType(engine, modelPointerToScriptValue, modelPointerFromScriptValue);
        qScriptRegisterMetaType(engine, meshPointerToScriptValue, meshPointerFromScriptValue);
        qScriptRegisterMetaType(engine, meshPartPointerToScriptValue, meshPartPointerFromScriptValue);

        return metaTypeIds.size();
    }
    // callback helper that lets C++ method signatures remain simple (ie: taking a single callback argument) while
    // still supporting extended Qt signal-like (scope, "methodName") and (scope, function(){}) "this" binding conventions
    QScriptValue jsBindCallback(QScriptValue callback) {
        if (callback.isObject() && callback.property("callback").isFunction()) {
            return callback;
        }
        auto engine = callback.engine();
        auto context = engine ? engine->currentContext() : nullptr;
        auto length = context ? context->argumentCount() : 0;
        QScriptValue scope = context ? context->thisObject() : QScriptValue::NullValue;
        QScriptValue method;
        qCInfo(graphics_scripting) << "jsBindCallback" << engine << length << scope.toQObject() << method.toString();
        int i = 0;
        for (; context && i < length; i++) {
            if (context->argument(i).strictlyEquals(callback)) {
                method = context->argument(i+1);
            }
        }
        if (method.isFunction() || method.isString()) {
            scope = callback;
        } else {
            method = callback;
        }
        qCInfo(graphics_scripting) << "scope:" << scope.toQObject() << "method:" << method.toString();
        return ::makeScopedHandlerObject(scope,  method);
    }
}

bool scriptable::GraphicsScriptingInterface::updateMeshPart(ScriptableMeshPointer mesh, ScriptableMeshPartPointer part) {
    Q_ASSERT(mesh);
    Q_ASSERT(part);
    Q_ASSERT(part->parentMesh);
    auto tmp = exportMeshPart(mesh, part->partIndex);
    if (part->parentMesh == mesh) {
        qCInfo(graphics_scripting) << "updateMeshPart -- update via clone" << mesh << part;
        tmp->replaceMeshData(part->cloneMeshPart());
        return false;
    } else {
        qCInfo(graphics_scripting) << "updateMeshPart -- update via inplace" << mesh << part;
        tmp->replaceMeshData(part);
        return true;
    }
}
