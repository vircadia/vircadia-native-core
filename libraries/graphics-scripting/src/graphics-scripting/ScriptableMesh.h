#pragma once

#include <glm/glm.hpp>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtCore/QUuid>
#include <QPointer>
#include <memory>

#include <DependencyManager.h>

#include <graphics-scripting/ScriptableModel.h>

#include <QtScript/QScriptable>
#include <QtScript/QScriptValue>

namespace graphics {
    class Mesh;
}
namespace gpu {
    class BufferView;
}
namespace scriptable {
    class ScriptableMeshPart;
    using ScriptableMeshPartPointer = QPointer<ScriptableMeshPart>;
    class ScriptableMesh : public QObject, QScriptable {
        Q_OBJECT
    public:
        Q_PROPERTY(quint32 numParts READ getNumParts)
        Q_PROPERTY(quint32 numAttributes READ getNumAttributes)
        Q_PROPERTY(quint32 numVertices READ getNumVertices)
        Q_PROPERTY(quint32 numIndices READ getNumIndices)
        Q_PROPERTY(QVariantMap metadata MEMBER _metadata)
        Q_PROPERTY(QVector<QString> attributeNames READ getAttributeNames)

        static QMap<QString,int> ATTRIBUTES;
        static std::map<QString, gpu::BufferView> gatherBufferViews(MeshPointer mesh, const QStringList& expandToMatchPositions = QStringList());

        ScriptableMesh& operator=(const ScriptableMesh& other)  { _model=other._model; _mesh=other._mesh; _metadata=other._metadata; return *this; };
        ScriptableMesh() : QObject(), _model(nullptr) {}
        ScriptableMesh(ScriptableModelPointer parent, scriptable::MeshPointer mesh) : QObject(), _model(parent), _mesh(mesh) {}
        ScriptableMesh(const ScriptableMesh& other) : QObject(), _model(other._model), _mesh(other._mesh), _metadata(other._metadata) {}
        ~ScriptableMesh() { qDebug() << "~ScriptableMesh" << this; }

        scriptable::MeshPointer getMeshPointer() const { return _mesh; }
   public slots:
        quint32 getNumParts() const;
        quint32 getNumVertices() const;
        quint32 getNumAttributes() const;
        quint32 getNumIndices() const { return 0;  }
        QVector<QString> getAttributeNames() const;

        QVariantMap getVertexAttributes(quint32 vertexIndex) const;
        QVariantMap getVertexAttributes(quint32 vertexIndex, QVector<QString> attributes) const;

        QVector<quint32> getIndices() const;
        QVector<quint32> findNearbyIndices(const glm::vec3& origin, float epsilon = 1e-6) const;
        QVariantMap getMeshExtents() const;
        bool setVertexAttributes(quint32 vertexIndex, QVariantMap attributes);
        QVariantMap scaleToFit(float unitScale);

        QVariantList getAttributeValues(const QString& attributeName) const;

        int _getSlotNumber(const QString& attributeName) const;

        QVariantMap translate(const glm::vec3& translation);
        QVariantMap scale(const glm::vec3& scale, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap rotateDegrees(const glm::vec3& eulerAngles, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap rotate(const glm::quat& rotation, const glm::vec3& origin = glm::vec3(NAN));
        QVariantMap transform(const glm::mat4& transform);

    public:
        operator bool() const { return _mesh != nullptr; }
        ScriptableModelPointer _model;
        scriptable::MeshPointer _mesh;
        QVariantMap _metadata;

    public slots:
        // QScriptEngine-specific wrappers
        QScriptValue mapAttributeValues(QScriptValue scopeOrCallback, QScriptValue methodOrName = QScriptValue());
        bool dedupeVertices(float epsilon = 1e-6);
        bool recalculateNormals();
        QScriptValue cloneMesh(bool recalcNormals = true);
        QScriptValue unrollVertices(bool recalcNormals = true);
        bool replaceMeshData(scriptable::ScriptableMeshPointer source, const QVector<QString>& attributeNames = QVector<QString>());
        QString toOBJ();
    };

    // TODO: part-specific wrapper for working with raw geometries
    class ScriptableMeshPart : public QObject {
        Q_OBJECT
    public:
        Q_PROPERTY(QString topology READ getTopology)
        Q_PROPERTY(quint32 numFaces READ getNumFaces)

        ScriptableMeshPart& operator=(const ScriptableMeshPart& view) { parentMesh=view.parentMesh; return *this; };
        ScriptableMeshPart(const ScriptableMeshPart& other) : parentMesh(other.parentMesh) {}
        ScriptableMeshPart()  {}
        ~ScriptableMeshPart() { qDebug() << "~ScriptableMeshPart" << this; }

    public slots:
        QString getTopology() const { return "triangles"; }
        quint32 getNumFaces() const { return parentMesh.getIndices().size() / 3; }
        QVector<quint32> getFace(quint32 faceIndex) const {
            auto inds = parentMesh.getIndices();
            return faceIndex+2 < (quint32)inds.size() ? inds.mid(faceIndex*3, 3) : QVector<quint32>();
        }

    public:
        scriptable::ScriptableMesh parentMesh;
        int partIndex;
    };

    class GraphicsScriptingInterface : public QObject {
        Q_OBJECT
    public:
        GraphicsScriptingInterface(QObject* parent = nullptr) : QObject(parent) {}
        GraphicsScriptingInterface(const GraphicsScriptingInterface& other) {}
        public slots:
            ScriptableMeshPart exportMeshPart(ScriptableMesh mesh, int part) { return {}; }
    };
}

Q_DECLARE_METATYPE(scriptable::ScriptableMeshPointer)
Q_DECLARE_METATYPE(QVector<scriptable::ScriptableMeshPointer>)
Q_DECLARE_METATYPE(scriptable::ScriptableMeshPartPointer)
Q_DECLARE_METATYPE(scriptable::GraphicsScriptingInterface)

// FIXME: MESHFACES: faces were supported in the original Model.* API -- are they still needed/used/useful for anything yet?
#include <memory>

namespace mesh {
    using uint32 = quint32;
    class MeshFace;
    using MeshFaces = QVector<mesh::MeshFace>;
    class MeshFace {
    public:
        MeshFace() {}
        MeshFace(QVector<mesh::uint32> vertexIndices) : vertexIndices(vertexIndices) {}
        ~MeshFace() {}

        QVector<mesh::uint32> vertexIndices;
        // TODO -- material...
    };
};

Q_DECLARE_METATYPE(mesh::MeshFace)
Q_DECLARE_METATYPE(QVector<mesh::MeshFace>)
Q_DECLARE_METATYPE(mesh::uint32)
Q_DECLARE_METATYPE(QVector<mesh::uint32>)
