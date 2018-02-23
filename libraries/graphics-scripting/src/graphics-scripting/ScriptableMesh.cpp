//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Forward.h"

#include "ScriptableMesh.h"

#include "BufferViewScripting.h"
#include "GraphicsScriptingUtil.h"
#include "OBJWriter.h"
#include <BaseScriptEngine.h>
#include <QtScript/QScriptValue>
#include <RegisteredMetaTypes.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <graphics/BufferViewHelpers.h>
#include <graphics/Geometry.h>

// #define SCRIPTABLE_MESH_DEBUG 1

scriptable::ScriptableMeshPart::ScriptableMeshPart(scriptable::ScriptableMeshPointer parentMesh, int partIndex)
    : QObject(), parentMesh(parentMesh), partIndex(partIndex)  {
    setObjectName(QString("%1.part[%2]").arg(parentMesh ? parentMesh->objectName() : "").arg(partIndex));
}

scriptable::ScriptableMesh::ScriptableMesh(const ScriptableMeshBase& other)
    : ScriptableMeshBase(other), QScriptable() {
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
#ifdef SCRIPTABLE_MESH_DEBUG
        qCDebug(graphics_scripting, "getTriangleIndices mesh %p", mesh.get());
#endif
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

QVariantMap scriptable::ScriptableMesh::getVertexAttributes(quint32 vertexIndex) const {
    return getVertexAttributes(vertexIndex, getAttributeNames());
}

bool scriptable::ScriptableMesh::setVertexAttributes(quint32 vertexIndex, QVariantMap attributes) {
    for (auto& a : buffer_helpers::gatherBufferViews(getMeshPointer())) {
        const auto& name = a.first;
        const auto& value = attributes.value(name);
        if (value.isValid()) {
            auto& view = a.second;
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

    // remove attributes only found on target mesh, unless user has explicitly specified the relevant attribute names
    if (attributeNames.isEmpty()) {
        auto attributeViews = buffer_helpers::gatherBufferViews(target);
        for (const auto& a : attributeViews) {
            auto slot = buffer_helpers::ATTRIBUTES[a.first];
            if (!attributes.contains(a.first)) {
#ifdef SCRIPTABLE_MESH_DEBUG
                qCInfo(graphics_scripting) << "ScriptableMesh::replaceMeshData -- pruning target attribute" << a.first << slot;
#endif
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
#ifdef SCRIPTABLE_MESH_DEBUG
        auto& before = target->getAttributeBuffer(slot);
#endif
        auto& input = source->getAttributeBuffer(slot);
        if (input.getNumElements() == 0) {
#ifdef SCRIPTABLE_MESH_DEBUG
            qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData buffer is empty -- pruning" << a << slot;
#endif
            target->removeAttribute(slot);
        } else {
#ifdef SCRIPTABLE_MESH_DEBUG
            if (before.getNumElements() == 0) {
                qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData target buffer is empty -- adding" << a << slot;
            } else {
                qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData target buffer exists -- updating" << a << slot;
            }
#endif
            target->addAttribute(slot, buffer_helpers::clone(input));
        }
#ifdef SCRIPTABLE_MESH_DEBUG
        auto& after = target->getAttributeBuffer(slot);
        qCInfo(graphics_scripting) << "ScriptableMeshPart::replaceMeshData" << a << slot << before.getNumElements() << " -> " << after.getNumElements();
#endif
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
        auto newView = buffer_helpers::resize(view, numUniqueVerts);
#ifdef SCRIPTABLE_MESH_DEBUG
        qCInfo(graphics_scripting) << "ScriptableMeshPart::dedupeVertices" << a.first << slot << view.getNumElements();
        qCInfo(graphics_scripting) << a.first << "before: #" << view.getNumElements() << "after: #" << newView.getNumElements();
#endif
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
    auto clone = buffer_helpers::cloneMesh(mesh);

    if (recalcNormals) {
        buffer_helpers::recalculateNormals(clone);
    }
    auto meshPointer = scriptable::make_scriptowned<scriptable::ScriptableMesh>(provider, model, clone, nullptr);
    return scriptable::ScriptableMeshPointer(meshPointer);
}

// note: we don't always want the JS side to prevent mesh data from being freed (hence weak pointers unless parented QObject)
scriptable::ScriptableMeshBase::ScriptableMeshBase(
    scriptable::WeakModelProviderPointer provider, scriptable::ScriptableModelBasePointer model, scriptable::WeakMeshPointer weakMesh, QObject* parent
    ) : QObject(parent), provider(provider), model(model), weakMesh(weakMesh) {
    if (parent) {
#ifdef SCRIPTABLE_MESH_DEBUG
        qCDebug(graphics_scripting) << "ScriptableMeshBase -- have parent QObject, creating strong neshref" << weakMesh.lock().get() << parent;
#endif
        strongMesh = weakMesh.lock();
    }
}

scriptable::ScriptableMeshBase::ScriptableMeshBase(scriptable::WeakMeshPointer weakMesh, QObject* parent) :
    scriptable::ScriptableMeshBase(scriptable::WeakModelProviderPointer(), nullptr, weakMesh, parent) {
}

scriptable::ScriptableMeshBase& scriptable::ScriptableMeshBase::operator=(const scriptable::ScriptableMeshBase& view) {
    provider = view.provider;
    model = view.model;
    weakMesh = view.weakMesh;
    strongMesh = view.strongMesh;
    return *this;
}

scriptable::ScriptableMeshBase::~ScriptableMeshBase() {
#ifdef SCRIPTABLE_MESH_DEBUG
    qCInfo(graphics_scripting) << "//~ScriptableMeshBase" << this << "strongMesh:"  << strongMesh.use_count() << "weakMesh:" << weakMesh.use_count();
#endif
    strongMesh.reset();
}

scriptable::ScriptableMesh::~ScriptableMesh() {
#ifdef SCRIPTABLE_MESH_DEBUG
    qCInfo(graphics_scripting) << "//~ScriptableMesh" << this << "strongMesh:"  << strongMesh.use_count() << "weakMesh:" << weakMesh.use_count();
#endif
    strongMesh.reset();
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
        return engine->newQObject(object, QScriptEngine::QtOwnership, QScriptEngine::ExcludeDeleteLater);
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
    QScriptValue jsBindCallback(QScriptValue value) {
        if (value.isObject() && value.property("callback").isFunction()) {
            // value is already a bound callback
            return value;
        }
        auto engine = value.engine();
        auto context = engine ? engine->currentContext() : nullptr;
        auto length = context ? context->argumentCount() : 0;
        QScriptValue scope = context ? context->thisObject() : QScriptValue::NullValue;
        QScriptValue method;
#ifdef SCRIPTABLE_MESH_DEBUG
        qCInfo(graphics_scripting) << "jsBindCallback" << engine << length << scope.toQObject() << method.toString();
#endif

        // find position in the incoming JS Function.arguments array (so we can test for the two-argument case)
        for (int i = 0; context && i < length; i++) {
            if (context->argument(i).strictlyEquals(value)) {
                method = context->argument(i+1);
            }
        }
        if (method.isFunction() || method.isString()) {
            // interpret as `API.func(..., scope, function callback(){})` or `API.func(..., scope, "methodName")`
            scope = value;
        } else {
            // interpret as `API.func(..., function callback(){})`
            method = value;
        }
#ifdef SCRIPTABLE_MESH_DEBUG
        qCInfo(graphics_scripting) << "scope:" << scope.toQObject() << "method:" << method.toString();
#endif
        return ::makeScopedHandlerObject(scope,  method);
    }
}

#include "ScriptableMesh.moc"
