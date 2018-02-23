//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "ScriptableModel.h"

#include <glm/glm.hpp>
#include <graphics/BufferViewHelpers.h>
#include <DependencyManager.h>

#include <memory>
#include <QPointer>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptable>

#include "GraphicsScriptingUtil.h"

namespace scriptable {
    class ScriptableMesh : public ScriptableMeshBase, QScriptable {
        Q_OBJECT
    public:
        Q_PROPERTY(uint32 numParts READ getNumParts)
        Q_PROPERTY(uint32 numAttributes READ getNumAttributes)
        Q_PROPERTY(uint32 numVertices READ getNumVertices)
        Q_PROPERTY(uint32 numIndices READ getNumIndices)
        Q_PROPERTY(QVector<QString> attributeNames READ getAttributeNames)
        Q_PROPERTY(QVector<scriptable::ScriptableMeshPartPointer> parts READ getMeshParts)
        Q_PROPERTY(bool valid READ hasValidMesh)
        Q_PROPERTY(bool strong READ hasValidStrongMesh)

        operator const ScriptableMeshBase*() const { return (qobject_cast<const scriptable::ScriptableMeshBase*>(this)); }

        ScriptableMesh(WeakModelProviderPointer provider, ScriptableModelBasePointer model, MeshPointer mesh, QObject* parent)
            : ScriptableMeshBase(provider, model, mesh, parent), QScriptable() { strongMesh = mesh; }
        ScriptableMesh(MeshPointer mesh, QObject* parent)
            : ScriptableMeshBase(WeakModelProviderPointer(), nullptr, mesh, parent), QScriptable() { strongMesh = mesh; }
        ScriptableMesh(const ScriptableMeshBase& other);
        ScriptableMesh(const ScriptableMesh& other) : ScriptableMeshBase(other), QScriptable() {};
        virtual ~ScriptableMesh();

        Q_INVOKABLE const scriptable::ScriptableModelPointer getParentModel() const { return qobject_cast<scriptable::ScriptableModel*>(model); }
        Q_INVOKABLE const scriptable::MeshPointer getOwnedMeshPointer() const { return strongMesh; }
        scriptable::ScriptableMeshPointer getSelf() const { return const_cast<scriptable::ScriptableMesh*>(this); }
        bool hasValidMesh() const { return !weakMesh.expired(); }
        bool hasValidStrongMesh() const { return (bool)strongMesh; }
   public slots:
        uint32 getNumParts() const;
        uint32 getNumVertices() const;
        uint32 getNumAttributes() const;
        uint32 getNumIndices() const;
        QVector<QString> getAttributeNames() const;
        QVector<scriptable::ScriptableMeshPartPointer> getMeshParts() const;

        QVariantMap getVertexAttributes(uint32 vertexIndex) const;
        QVariantMap getVertexAttributes(uint32 vertexIndex, QVector<QString> attributes) const;

        QVector<uint32> getIndices() const;
        QVector<uint32> findNearbyIndices(const glm::vec3& origin, float epsilon = 1e-6) const;
        QVariantMap getMeshExtents() const;
        bool setVertexAttributes(uint32 vertexIndex, QVariantMap attributes);

        QVariantList getAttributeValues(const QString& attributeName) const;

        int _getSlotNumber(const QString& attributeName) const;

        scriptable::ScriptableMeshPointer cloneMesh(bool recalcNormals = false);
    public:
        operator bool() const { return !weakMesh.expired(); }

    public slots:
        // QScriptEngine-specific wrappers
        uint32 mapAttributeValues(QScriptValue callback);
    };

    // TODO: part-specific wrapper for working with raw geometries
    class ScriptableMeshPart : public QObject, QScriptable {
        Q_OBJECT
    public:
        Q_PROPERTY(uint32 partIndex MEMBER partIndex CONSTANT)
        Q_PROPERTY(int numElementsPerFace MEMBER _elementsPerFace CONSTANT)
        Q_PROPERTY(QString topology MEMBER _topology CONSTANT)

        Q_PROPERTY(uint32 numFaces READ getNumFaces)
        Q_PROPERTY(uint32 numAttributes READ getNumAttributes)
        Q_PROPERTY(uint32 numVertices READ getNumVertices)
        Q_PROPERTY(uint32 numIndices READ getNumIndices)
        Q_PROPERTY(QVector<QString> attributeNames READ getAttributeNames)

        ScriptableMeshPart(scriptable::ScriptableMeshPointer parentMesh, int partIndex);
        ScriptableMeshPart& operator=(const ScriptableMeshPart& view) { parentMesh=view.parentMesh; return *this; };
        ScriptableMeshPart(const ScriptableMeshPart& other) : QObject(other.parent()), QScriptable(), parentMesh(other.parentMesh), partIndex(other.partIndex) {}

    public slots:
        scriptable::ScriptableMeshPointer getParentMesh() const { return parentMesh; }
        uint32 getNumAttributes() const { return parentMesh ? parentMesh->getNumAttributes() : 0; }
        uint32 getNumVertices() const { return parentMesh ? parentMesh->getNumVertices() : 0; }
        uint32 getNumIndices() const { return parentMesh ? parentMesh->getNumIndices() : 0; }
        uint32 getNumFaces() const { return parentMesh ? parentMesh->getNumIndices() / _elementsPerFace : 0; }
        QVector<QString> getAttributeNames() const { return parentMesh ? parentMesh->getAttributeNames() : QVector<QString>(); }
        QVector<uint32> getFace(uint32 faceIndex) const {
            auto inds = parentMesh ? parentMesh->getIndices() : QVector<uint32>();
            return faceIndex+2 < (uint32)inds.size() ? inds.mid(faceIndex*3, 3) : QVector<uint32>();
        }
        QVariantMap scaleToFit(float unitScale);
        QVariantMap translate(const glm::vec3& translation);
        QVariantMap scale(const glm::vec3& scale, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap rotateDegrees(const glm::vec3& eulerAngles, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap rotate(const glm::quat& rotation, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap transform(const glm::mat4& transform);

        bool unrollVertices(bool recalcNormals = false);
        bool dedupeVertices(float epsilon = 1e-6);
        bool recalculateNormals() { return buffer_helpers::recalculateNormals(getMeshPointer()); }

        bool replaceMeshData(scriptable::ScriptableMeshPartPointer source, const QVector<QString>& attributeNames = QVector<QString>());
        scriptable::ScriptableMeshPartPointer cloneMeshPart(bool recalcNormals = false) {
            if (parentMesh) {
                if (auto clone = parentMesh->cloneMesh(recalcNormals)) {
                    return clone->getMeshParts().value(partIndex);
                }
            }
            return nullptr;
        }
        QString toOBJ();

    public slots:
        // QScriptEngine-specific wrappers
        uint32 mapAttributeValues(QScriptValue callback);

    public:
        scriptable::ScriptableMeshPointer parentMesh;
        uint32 partIndex;
    protected:
        int _elementsPerFace{ 3 };
        QString _topology{ "triangles" };
        scriptable::MeshPointer getMeshPointer() const { return parentMesh ? parentMesh->getMeshPointer() : nullptr; }
    };

    // callback helper that lets C++ method signatures remain simple (ie: taking a single callback argument) while
    // still supporting extended Qt signal-like (scope, "methodName") and (scope, function(){}) "this" binding conventions
    QScriptValue jsBindCallback(QScriptValue callback);

    // derive a corresponding C++ class instance from the current script engine's thisObject
    template <typename T> T this_qobject_cast(QScriptEngine* engine);
}

Q_DECLARE_METATYPE(scriptable::ScriptableMeshPointer)
Q_DECLARE_METATYPE(QVector<scriptable::ScriptableMeshPointer>)
Q_DECLARE_METATYPE(scriptable::ScriptableMeshPartPointer)
Q_DECLARE_METATYPE(QVector<scriptable::ScriptableMeshPartPointer>)
Q_DECLARE_METATYPE(scriptable::uint32)
Q_DECLARE_METATYPE(QVector<scriptable::uint32>)
