#pragma once

#include <glm/glm.hpp>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtCore/QUuid>
#include <memory>

#include <DependencyManager.h>

#include <graphics-scripting/ScriptableModel.h>

namespace graphics {
    class Mesh;
}
namespace gpu {
    class BufferView;
}
namespace scriptable {
    class ScriptableMesh : public QObject, public std::enable_shared_from_this<ScriptableMesh> {
        Q_OBJECT
    public:
        ScriptableModelPointer _model;
        scriptable::MeshPointer _mesh;
        QVariantMap _metadata;
    ScriptableMesh() : QObject() {}
    ScriptableMesh(ScriptableModelPointer parent, scriptable::MeshPointer mesh) : QObject(), _model(parent), _mesh(mesh) {}
    ScriptableMesh(const ScriptableMesh& other) : QObject(), _model(other._model), _mesh(other._mesh), _metadata(other._metadata) {}
        ~ScriptableMesh() { qDebug() << "~ScriptableMesh" << this; }
        Q_PROPERTY(quint32 numParts READ getNumParts)
        Q_PROPERTY(quint32 numAttributes READ getNumAttributes)
        Q_PROPERTY(quint32 numVertices READ getNumVertices)
        Q_PROPERTY(quint32 numIndices READ getNumIndices)
        Q_PROPERTY(QVector<QString> attributeNames READ getAttributeNames)

        virtual scriptable::MeshPointer getMeshPointer() const { return _mesh; }
        Q_INVOKABLE virtual quint32 getNumParts() const;
        Q_INVOKABLE virtual quint32 getNumVertices() const;
        Q_INVOKABLE virtual quint32 getNumAttributes() const;
        Q_INVOKABLE virtual quint32 getNumIndices() const { return 0;  }
        Q_INVOKABLE virtual QVector<QString> getAttributeNames() const;
        Q_INVOKABLE virtual QVariantMap getVertexAttributes(quint32 vertexIndex) const;
        Q_INVOKABLE virtual QVariantMap getVertexAttributes(quint32 vertexIndex, QVector<QString> attributes) const;
        
        Q_INVOKABLE virtual QVector<quint32> getIndices() const;
        Q_INVOKABLE virtual QVector<quint32> findNearbyIndices(const glm::vec3& origin, float epsilon = 1e-6) const;
        Q_INVOKABLE virtual QVariantMap getMeshExtents() const;
        Q_INVOKABLE virtual bool setVertexAttributes(quint32 vertexIndex, QVariantMap attributes);
        Q_INVOKABLE virtual QVariantMap scaleToFit(float unitScale);

    static QMap<QString,int> ATTRIBUTES;
    static std::map<QString, gpu::BufferView> gatherBufferViews(MeshPointer mesh, const QStringList& expandToMatchPositions = QStringList());

    Q_INVOKABLE QVariantList getAttributeValues(const QString& attributeName) const;

    Q_INVOKABLE int _getSlotNumber(const QString& attributeName) const;

    QVariantMap translate(const glm::vec3& translation);
    QVariantMap scale(const glm::vec3& scale, const glm::vec3& origin = glm::vec3(NAN));
    QVariantMap rotateDegrees(const glm::vec3& eulerAngles, const glm::vec3& origin = glm::vec3(NAN));
    QVariantMap rotate(const glm::quat& rotation, const glm::vec3& origin = glm::vec3(NAN));
    Q_INVOKABLE QVariantMap transform(const glm::mat4& transform);
    };

    // TODO: for now this is a part-specific wrapper around ScriptableMesh
    class ScriptableMeshPart : public ScriptableMesh {
        Q_OBJECT
    public:
        ScriptableMeshPart& operator=(const ScriptableMeshPart& view) { _model=view._model; _mesh=view._mesh; return *this; };
        ScriptableMeshPart(const ScriptableMeshPart& other) : ScriptableMesh(other._model, other._mesh) {}
        ScriptableMeshPart() : ScriptableMesh(nullptr, nullptr) {}
        ~ScriptableMeshPart() { qDebug() << "~ScriptableMeshPart" << this; }
        ScriptableMeshPart(ScriptableMeshPointer mesh) : ScriptableMesh(mesh->_model, mesh->_mesh) {}
        Q_PROPERTY(QString topology READ getTopology)
        Q_PROPERTY(quint32 numFaces READ getNumFaces)

        scriptable::MeshPointer parentMesh;
        int partIndex;
        QString getTopology() const { return "triangles"; }         
        Q_INVOKABLE virtual quint32 getNumFaces() const { return getIndices().size() / 3; }
        Q_INVOKABLE virtual QVector<quint32> getFace(quint32 faceIndex) const {
            auto inds = getIndices();
            return faceIndex+2 < (quint32)inds.size() ? inds.mid(faceIndex*3, 3) : QVector<quint32>();
        }
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

Q_DECLARE_METATYPE(scriptable::ScriptableMesh)
Q_DECLARE_METATYPE(scriptable::ScriptableMeshPointer)
Q_DECLARE_METATYPE(QVector<scriptable::ScriptableMeshPointer>)
Q_DECLARE_METATYPE(scriptable::ScriptableMeshPart)
Q_DECLARE_METATYPE(scriptable::GraphicsScriptingInterface)

// FIXME: faces were supported in the original Model.* API -- are they still needed/used/useful for anything yet?
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
