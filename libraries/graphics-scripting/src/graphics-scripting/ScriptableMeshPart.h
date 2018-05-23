//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "ScriptableMesh.h"

namespace scriptable {
    /**jsdoc
     * @typedef {object} Graphics.MeshPart
     * @property {boolean} valid
     * @property {number} partIndex - The part index (within the containing Mesh).
     * @property {number} firstVertexIndex
     * @property {number} baseVertexIndex
     * @property {number} lastVertexIndex
     * @property {Graphics.Topology} topology - element interpretation (currently only 'triangles' is supported).
     * @property {string[]} attributeNames - Vertex attribute names (color, normal, etc.)
     * @property {number} numIndices - Number of vertex indices that this mesh part refers to.
     * @property {number} numVerticesPerFace - Number of vertices per face (eg: 3 when topology is 'triangles').
     * @property {number} numFaces - Number of faces represented by the mesh part (numIndices / numVerticesPerFace).
     * @property {number} numVertices - Total number of vertices in the containing Mesh.
     * @property {number} numAttributes - Number of currently defined vertex attributes.
     * @property {object} extents
     * @property {object} bufferFormats
     */

    class ScriptableMeshPart : public QObject, QScriptable {
        Q_OBJECT
        Q_PROPERTY(bool valid READ isValid)
        Q_PROPERTY(glm::uint32 partIndex MEMBER partIndex CONSTANT)
        Q_PROPERTY(glm::uint32 firstVertexIndex READ getFirstVertexIndex WRITE setFirstVertexIndex)
        Q_PROPERTY(glm::uint32 baseVertexIndex READ getBaseVertexIndex WRITE setBaseVertexIndex)
        Q_PROPERTY(glm::uint32 lastVertexIndex READ getLastVertexIndex WRITE setLastVertexIndex)
        Q_PROPERTY(int numVerticesPerFace READ getTopologyLength)
        // NOTE: making read-only for now (see also GraphicsScriptingInterface::newMesh and MeshPartPayload::drawCall)
        Q_PROPERTY(graphics::Mesh::Topology topology READ getTopology)

        Q_PROPERTY(glm::uint32 numFaces READ getNumFaces)
        Q_PROPERTY(glm::uint32 numAttributes READ getNumAttributes)
        Q_PROPERTY(glm::uint32 numVertices READ getNumVertices)
        Q_PROPERTY(glm::uint32 numIndices READ getNumIndices WRITE setNumIndices)

        Q_PROPERTY(QVariantMap extents READ getPartExtents)
        Q_PROPERTY(QVector<QString> attributeNames READ getAttributeNames)
        Q_PROPERTY(QVariantMap bufferFormats READ getBufferFormats)

    public:
        ScriptableMeshPart(scriptable::ScriptableMeshPointer parentMesh, int partIndex);
        ScriptableMeshPart& operator=(const ScriptableMeshPart& view) { parentMesh=view.parentMesh; return *this; };
        ScriptableMeshPart(const ScriptableMeshPart& other) : QObject(other.parent()), QScriptable(), parentMesh(other.parentMesh), partIndex(other.partIndex) {}
        bool isValid() const { auto mesh = getMeshPointer(); return mesh && partIndex < mesh->getNumParts(); }

    public slots:
        QVector<glm::uint32> getIndices() const;
        bool setIndices(const QVector<glm::uint32>& indices);
        QVector<glm::uint32> findNearbyPartVertexIndices(const glm::vec3& origin, float epsilon = 1e-6) const;
        QVariantList queryVertexAttributes(QVariant selector) const;
        QVariantMap getVertexAttributes(glm::uint32 vertexIndex) const;
        bool setVertexAttributes(glm::uint32 vertexIndex, const QVariantMap& attributeValues);

        QVariant getVertexProperty(glm::uint32 vertexIndex, const QString& attributeName) const;
        bool setVertexProperty(glm::uint32 vertexIndex, const QString& attributeName, const QVariant& attributeValues);

        QVector<glm::uint32> getFace(glm::uint32 faceIndex) const;

        QVariantMap scaleToFit(float unitScale);
        QVariantMap translate(const glm::vec3& translation);
        QVariantMap scale(const glm::vec3& scale, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap rotateDegrees(const glm::vec3& eulerAngles, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap rotate(const glm::quat& rotation, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap transform(const glm::mat4& transform);

        glm::uint32 addAttribute(const QString& attributeName, const QVariant& defaultValue = QVariant());
        glm::uint32 fillAttribute(const QString& attributeName, const QVariant& value);
        bool removeAttribute(const QString& attributeName);
        bool dedupeVertices(float epsilon = 1e-6);

        scriptable::ScriptableMeshPointer getParentMesh() const { return parentMesh; }

        bool replaceMeshPartData(scriptable::ScriptableMeshPartPointer source, const QVector<QString>& attributeNames = QVector<QString>());
        scriptable::ScriptableMeshPartPointer cloneMeshPart();

        QString toOBJ();

        // QScriptEngine-specific wrappers
        glm::uint32 updateVertexAttributes(QScriptValue callback);
        glm::uint32 forEachVertex(QScriptValue callback);

        bool isValidIndex(glm::uint32 vertexIndex, const QString& attributeName = QString()) const;
    public:
        scriptable::ScriptableMeshPointer parentMesh;
        glm::uint32 partIndex;

    protected:
        const graphics::Mesh::Part& getMeshPart() const;
        scriptable::MeshPointer getMeshPointer() const { return parentMesh ? parentMesh->getMeshPointer() : nullptr; }
        QVariantMap getBufferFormats() { return isValid()  ? parentMesh->getBufferFormats() : QVariantMap(); }
        glm::uint32 getNumAttributes() const { return isValid() ? parentMesh->getNumAttributes() : 0; }

        bool setTopology(graphics::Mesh::Topology topology);
        graphics::Mesh::Topology getTopology() const { return isValid() ? getMeshPart()._topology : graphics::Mesh::Topology(); }
        glm::uint32 getTopologyLength() const;
        glm::uint32 getNumIndices() const { return isValid() ? getMeshPart()._numIndices : 0; }
        bool setNumIndices(glm::uint32 numIndices) { return setLastVertexIndex(getFirstVertexIndex() + numIndices); }
        glm::uint32 getNumVertices() const { return isValid() ? parentMesh->getNumVertices() : 0; }

        bool setFirstVertexIndex(glm::uint32 vertexIndex);
        glm::uint32 getFirstVertexIndex() const { return isValid() ? getMeshPart()._startIndex : 0; }
        bool setLastVertexIndex(glm::uint32 vertexIndex);
        glm::uint32 getLastVertexIndex() const { return isValid() ? getFirstVertexIndex() + getNumIndices() - 1 : 0; }
        bool setBaseVertexIndex(glm::uint32 vertexIndex);
        glm::uint32 getBaseVertexIndex() const { return isValid() ? getMeshPart()._baseVertex : 0; }

        glm::uint32 getNumFaces() const { return getNumIndices() / getTopologyLength(); }
        QVector<QString> getAttributeNames() const { return isValid() ? parentMesh->getAttributeNames() : QVector<QString>(); }
        QVariantMap getPartExtents() const;
    };
}

Q_DECLARE_METATYPE(scriptable::ScriptableMeshPartPointer)
Q_DECLARE_METATYPE(QVector<scriptable::ScriptableMeshPartPointer>)
