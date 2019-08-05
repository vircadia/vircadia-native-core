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

#include <graphics/Geometry.h>

namespace scriptable {
    /**jsdoc
     * @typedef {object} Graphics.Mesh
     * @property {Graphics.MeshPart[]} parts - Array of submesh part references.
     * @property {string[]} attributeNames - Vertex attribute names (color, normal, etc.)
     * @property {number} numParts - The number of parts contained in the mesh.
     * @property {number} numIndices - Total number of vertex indices in the mesh.
     * @property {number} numVertices - Total number of vertices in the Mesh.
     * @property {number} numAttributes - Number of currently defined vertex attributes.
     * @property {boolean} valid
     * @property {boolean} strong
     * @property {object} extents
     * @property {object} bufferFormats
     */
    class ScriptableMesh : public ScriptableMeshBase, QScriptable {
        Q_OBJECT
    public:
        Q_PROPERTY(glm::uint32 numParts READ getNumParts)
        Q_PROPERTY(glm::uint32 numAttributes READ getNumAttributes)
        Q_PROPERTY(glm::uint32 numVertices READ getNumVertices)
        Q_PROPERTY(glm::uint32 numIndices READ getNumIndices)
        Q_PROPERTY(QVector<QString> attributeNames READ getAttributeNames)
        Q_PROPERTY(QVector<scriptable::ScriptableMeshPartPointer> parts READ getMeshParts)
        Q_PROPERTY(bool valid READ isValid)
        Q_PROPERTY(bool strong READ hasValidStrongMesh)
        Q_PROPERTY(QVariantMap extents READ getMeshExtents)
        Q_PROPERTY(QVariantMap bufferFormats READ getBufferFormats)
        QVariantMap getBufferFormats() const;

        operator const ScriptableMeshBase*() const { return (qobject_cast<const scriptable::ScriptableMeshBase*>(this)); }

        ScriptableMesh(WeakModelProviderPointer provider, ScriptableModelBasePointer model, MeshPointer mesh, QObject* parent)
            : ScriptableMeshBase(provider, model, mesh, parent), QScriptable() { strongMesh = mesh; }
        ScriptableMesh(MeshPointer mesh, QObject* parent)
            : ScriptableMeshBase(WeakModelProviderPointer(), nullptr, mesh, parent), QScriptable() { strongMesh = mesh; }
        ScriptableMesh(const ScriptableMeshBase& other);
        ScriptableMesh(const ScriptableMesh& other) : ScriptableMeshBase(other), QScriptable() {};
        virtual ~ScriptableMesh();

        const scriptable::MeshPointer getOwnedMeshPointer() const { return strongMesh; }
        scriptable::ScriptableMeshPointer getSelf() const { return const_cast<scriptable::ScriptableMesh*>(this); }
        bool isValid() const { return !weakMesh.expired(); }
        bool hasValidStrongMesh() const { return (bool)strongMesh; }
        glm::uint32 getNumParts() const;
        glm::uint32 getNumVertices() const;
        glm::uint32 getNumAttributes() const;
        glm::uint32 getNumIndices() const;
        QVector<QString> getAttributeNames() const;
        QVector<scriptable::ScriptableMeshPartPointer> getMeshParts() const;
        QVariantMap getMeshExtents() const;

        operator bool() const { return !weakMesh.expired(); }
        int getSlotNumber(const QString& attributeName) const;

    public slots:
        const scriptable::ScriptableModelPointer getParentModel() const { return qobject_cast<scriptable::ScriptableModel*>(model); }
        QVector<glm::uint32> getIndices() const;
        QVector<glm::uint32> findNearbyVertexIndices(const glm::vec3& origin, float epsilon = 1e-6) const;

        glm::uint32 addAttribute(const QString& attributeName, const QVariant& defaultValue = QVariant());
        glm::uint32 fillAttribute(const QString& attributeName, const QVariant& value);
        bool removeAttribute(const QString& attributeName);

        QVariantList queryVertexAttributes(QVariant selector) const;
        QVariantMap getVertexAttributes(glm::uint32 vertexIndex) const;
        bool setVertexAttributes(glm::uint32 vertexIndex, const QVariantMap& attributeValues);

        QVariant getVertexProperty(glm::uint32 vertexIndex, const QString& attributeName) const;
        bool setVertexProperty(glm::uint32 vertexIndex, const QString& attributeName, const QVariant& value);

        scriptable::ScriptableMeshPointer cloneMesh();

        // QScriptEngine-specific wrappers
        glm::uint32 updateVertexAttributes(QScriptValue callback);
        glm::uint32 forEachVertex(QScriptValue callback);
        bool isValidIndex(glm::uint32 vertexIndex, const QString& attributeName = QString()) const;
    };

}

Q_DECLARE_METATYPE(scriptable::ScriptableMeshPointer)
Q_DECLARE_METATYPE(QVector<scriptable::ScriptableMeshPointer>)
