//
//  MetavoxelSystem.h
//  interface/src
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelSystem_h
#define hifi_MetavoxelSystem_h

#include <QList>
#include <QOpenGLBuffer>
#include <QReadWriteLock>
#include <QVector>

#include <glm/glm.hpp>

#include <MetavoxelClientManager.h>

#include "renderer/ProgramObject.h"

class HeightfieldBaseLayerBatch;
class HeightfieldRendererNode;
class HeightfieldSplatBatch;
class HermiteBatch;
class Model;
class VoxelBatch;
class VoxelSplatBatch;

/// Renders a metavoxel tree.
class MetavoxelSystem : public MetavoxelClientManager {
    Q_OBJECT

public:

    class NetworkSimulation {
    public:
        float dropRate;
        float repeatRate;
        int minimumDelay;
        int maximumDelay;
        int bandwidthLimit;
        
        NetworkSimulation(float dropRate = 0.0f, float repeatRate = 0.0f, int minimumDelay = 0,
            int maximumDelay = 0, int bandwidthLimit = 0);
    };

    virtual ~MetavoxelSystem();

    virtual void init();

    virtual MetavoxelLOD getLOD();

    const Frustum& getFrustum() const { return _frustum; }
    
    void setNetworkSimulation(const NetworkSimulation& simulation);
    NetworkSimulation getNetworkSimulation();
    
    const AttributePointer& getHeightfieldBufferAttribute() { return _heightfieldBufferAttribute; }
    const AttributePointer& getVoxelBufferAttribute() { return _voxelBufferAttribute; }
    
    void simulate(float deltaTime);
    void render();

    void renderHeightfieldCursor(const glm::vec3& position, float radius);

    void renderVoxelCursor(const glm::vec3& position, float radius);

    bool findFirstRayVoxelIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance);

    Q_INVOKABLE void paintHeightfieldColor(const glm::vec3& position, float radius, const QColor& color);

    Q_INVOKABLE void paintHeightfieldMaterial(const glm::vec3& position, float radius, const SharedObjectPointer& material);
        
    Q_INVOKABLE void paintVoxelColor(const glm::vec3& position, float radius, const QColor& color);
    
    Q_INVOKABLE void paintVoxelMaterial(const glm::vec3& position, float radius, const SharedObjectPointer& material);
        
    Q_INVOKABLE void setVoxelColor(const SharedObjectPointer& spanner, const QColor& color);
        
    Q_INVOKABLE void setVoxelMaterial(const SharedObjectPointer& spanner, const SharedObjectPointer& material);
    
    void addHeightfieldBaseBatch(const HeightfieldBaseLayerBatch& batch) { _heightfieldBaseBatches.append(batch); }
    void addHeightfieldSplatBatch(const HeightfieldSplatBatch& batch) { _heightfieldSplatBatches.append(batch); }
    
    void addVoxelBaseBatch(const VoxelBatch& batch) { _voxelBaseBatches.append(batch); }
    void addVoxelSplatBatch(const VoxelSplatBatch& batch) { _voxelSplatBatches.append(batch); }
    
    void addHermiteBatch(const HermiteBatch& batch) { _hermiteBatches.append(batch); }
    
signals:

    void rendering();

public slots:

    void refreshVoxelData();

protected:

    Q_INVOKABLE void applyMaterialEdit(const MetavoxelEditMessage& message, bool reliable = false);

    virtual MetavoxelClient* createClient(const SharedNodePointer& node);

private:
    
    void guideToAugmented(MetavoxelVisitor& visitor, bool render = false);
    
    AttributePointer _heightfieldBufferAttribute;
    AttributePointer _voxelBufferAttribute;
    
    MetavoxelLOD _lod;
    QReadWriteLock _lodLock;
    Frustum _frustum;
    
    NetworkSimulation _networkSimulation;
    QReadWriteLock _networkSimulationLock;
    
    QVector<HeightfieldBaseLayerBatch> _heightfieldBaseBatches;
    QVector<HeightfieldSplatBatch> _heightfieldSplatBatches;
    QVector<VoxelBatch> _voxelBaseBatches;
    QVector<VoxelSplatBatch> _voxelSplatBatches;
    QVector<HermiteBatch> _hermiteBatches;
    
    ProgramObject _baseHeightfieldProgram;
    int _baseHeightScaleLocation;
    int _baseColorScaleLocation;
    
    class SplatLocations {
    public:
        int heightScale;
        int textureScale;
        int splatTextureOffset;
        int splatTextureScalesS;
        int splatTextureScalesT;
        int textureValueMinima;
        int textureValueMaxima;
        int materials;
        int materialWeights;
    };
    
    ProgramObject _splatHeightfieldProgram;
    SplatLocations _splatHeightfieldLocations;
    
    int _splatHeightScaleLocation;
    int _splatTextureScaleLocation;
    int _splatTextureOffsetLocation;
    int _splatTextureScalesSLocation;
    int _splatTextureScalesTLocation;
    int _splatTextureValueMinimaLocation;
    int _splatTextureValueMaximaLocation;
    
    ProgramObject _heightfieldCursorProgram;
    
    ProgramObject _baseVoxelProgram;
    ProgramObject _splatVoxelProgram;
    SplatLocations _splatVoxelLocations;
    
    ProgramObject _voxelCursorProgram;
    
    static void loadSplatProgram(const char* type, ProgramObject& program, SplatLocations& locations);
};

/// Base class for heightfield batches.
class HeightfieldBatch {
public:
    QOpenGLBuffer* vertexBuffer;
    QOpenGLBuffer* indexBuffer;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    int vertexCount;
    int indexCount;
    GLuint heightTextureID;
    glm::vec4 heightScale;
};

/// A batch containing a heightfield base layer. 
class HeightfieldBaseLayerBatch : public HeightfieldBatch {
public:
    GLuint colorTextureID;
    glm::vec2 colorScale;
};

/// A batch containing a heightfield splat.
class HeightfieldSplatBatch : public HeightfieldBatch {
public:
    GLuint materialTextureID;
    glm::vec2 textureScale;
    glm::vec2 splatTextureOffset;
    int splatTextureIDs[4];
    glm::vec4 splatTextureScalesS;
    glm::vec4 splatTextureScalesT;
    int materialIndex;
};

/// Base class for voxel batches.
class VoxelBatch {
public:
    QOpenGLBuffer* vertexBuffer;
    QOpenGLBuffer* indexBuffer;
    int vertexCount;
    int indexCount;
};

/// A batch containing a voxel splat.
class VoxelSplatBatch : public VoxelBatch {
public:
    int splatTextureIDs[4];
    glm::vec4 splatTextureScalesS;
    glm::vec4 splatTextureScalesT;
    int materialIndex;
};

/// A batch containing Hermite data for debugging.
class HermiteBatch {
public:
    QOpenGLBuffer* vertexBuffer;
    int vertexCount;
};

/// Generic abstract base class for objects that handle a signal.
class SignalHandler : public QObject {
    Q_OBJECT
    
public slots:
    
    virtual void handle() = 0;
};

/// Simple throttle for limiting bandwidth on a per-second basis.
class Throttle {
public:
    
    Throttle();
    
    /// Sets the per-second limit.
    void setLimit(int limit) { _limit = limit; }
    
    /// Determines whether the message with the given size should be throttled (discarded).  If not, registers the message
    /// as having been processed (i.e., contributing to later throttling).
    bool shouldThrottle(int bytes);

private:
    
    int _limit;
    int _total;
    
    typedef QPair<qint64, int> Bucket;
    QList<Bucket> _buckets;
};

/// A client session associated with a single server.
class MetavoxelSystemClient : public MetavoxelClient {
    Q_OBJECT    
    
public:
    
    MetavoxelSystemClient(const SharedNodePointer& node, MetavoxelUpdater* updater);
    
    Q_INVOKABLE void setAugmentedData(const MetavoxelData& data);
    
    /// Returns a copy of the augmented data.  This function is thread-safe.
    MetavoxelData getAugmentedData();
    
    void setRenderedAugmentedData(const MetavoxelData& data) { _renderedAugmentedData = data; }

    virtual int parseData(const QByteArray& packet);

    Q_INVOKABLE void refreshVoxelData();
    
protected:
    
    virtual void dataChanged(const MetavoxelData& oldData);
    virtual void sendDatagram(const QByteArray& data);

private:
    
    MetavoxelData _augmentedData;
    MetavoxelData _renderedAugmentedData;
    QReadWriteLock _augmentedDataLock;
    
    Throttle _sendThrottle;
    Throttle _receiveThrottle;
};

/// Base class for cached static buffers.
class BufferData : public QSharedData {
public:
    
    virtual ~BufferData();

    virtual void render(bool cursor = false) = 0;
};

typedef QExplicitlySharedDataPointer<BufferData> BufferDataPointer;

/// Describes contents of a vertex in a voxel buffer.
class VoxelPoint {
public:
    glm::vec3 vertex;
    quint8 color[3];
    char normal[3];
    quint8 materials[4];
    quint8 materialWeights[4];
    
    void setNormal(const glm::vec3& normal);
};

/// A container for a coordinate within a voxel block.
class VoxelCoord {
public:
    QRgb encoded;
    
    VoxelCoord(QRgb encoded) : encoded(encoded) { }
    
    bool operator==(const VoxelCoord& other) const { return encoded == other.encoded; }
};

inline uint qHash(const VoxelCoord& coord, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(qRed(coord.encoded) + 257 * (qGreen(coord.encoded) + 263 * qBlue(coord.encoded)), seed);
}

/// Contains the information necessary to render a voxel block.
class VoxelBuffer : public BufferData {
public:
    
    VoxelBuffer(const QVector<VoxelPoint>& vertices, const QVector<int>& indices, const QVector<glm::vec3>& hermite,
        const QMultiHash<VoxelCoord, int>& quadIndices, int size, const QVector<SharedObjectPointer>& materials =
            QVector<SharedObjectPointer>());

    /// Finds the first intersection between the described ray and the voxel data.
    /// \param entry the entry point of the ray in relative coordinates, from (0, 0, 0) to (1, 1, 1)
    bool findFirstRayIntersection(const glm::vec3& entry, const glm::vec3& origin,
        const glm::vec3& direction, float& distance) const;
        
    virtual void render(bool cursor = false);

private:
    
    QVector<VoxelPoint> _vertices;
    QVector<int> _indices;
    QVector<glm::vec3> _hermite;
    QMultiHash<VoxelCoord, int> _quadIndices;
    int _size;
    int _vertexCount;
    int _indexCount;
    int _hermiteCount;
    QOpenGLBuffer _vertexBuffer;
    QOpenGLBuffer _indexBuffer;
    QOpenGLBuffer _hermiteBuffer;
    QVector<SharedObjectPointer> _materials;
    QVector<NetworkTexturePointer> _networkTextures;
};

/// A client-side attribute that stores renderable buffers.
class BufferDataAttribute : public InlineAttribute<BufferDataPointer> {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE BufferDataAttribute(const QString& name = QString());
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
    
    virtual AttributeValue inherit(const AttributeValue& parentValue) const;
};

/// Renders metavoxels as points.
class DefaultMetavoxelRendererImplementation : public MetavoxelRendererImplementation {
    Q_OBJECT

public:
    
    Q_INVOKABLE DefaultMetavoxelRendererImplementation();
    
    virtual void augment(MetavoxelData& data, const MetavoxelData& previous, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void simulate(MetavoxelData& data, float deltaTime, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod);
};

/// Renders spheres.
class SphereRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE SphereRenderer();
    
    virtual void render(const MetavoxelLOD& lod = MetavoxelLOD(), bool contained = false, bool cursor = false);
};

/// Renders cuboids.
class CuboidRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE CuboidRenderer();
    
    virtual void render(const MetavoxelLOD& lod = MetavoxelLOD(), bool contained = false, bool cursor = false);
};

/// Renders static models.
class StaticModelRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE StaticModelRenderer();
    
    virtual void init(Spanner* spanner);
    virtual void simulate(float deltaTime);
    virtual void render(const MetavoxelLOD& lod = MetavoxelLOD(), bool contained = false, bool cursor = false);
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;

private slots:

    void applyTranslation(const glm::vec3& translation);
    void applyRotation(const glm::quat& rotation);
    void applyScale(float scale);
    void applyURL(const QUrl& url);

private:
    
    Model* _model;
};

typedef QExplicitlySharedDataPointer<HeightfieldRendererNode> HeightfieldRendererNodePointer;

/// Renders heightfields.
class HeightfieldRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE HeightfieldRenderer();
    
    virtual void init(Spanner* spanner);
    virtual void render(const MetavoxelLOD& lod = MetavoxelLOD(), bool contained = false, bool cursor = false);

private slots:

    void updateRoot();

private:
    
    HeightfieldRendererNodePointer _root;
};

/// A node in the heightfield renderer quadtree.
class HeightfieldRendererNode : public QSharedData {
public:
    
    static const int CHILD_COUNT = 4;
    
    HeightfieldRendererNode(const HeightfieldNodePointer& heightfieldNode);
    virtual ~HeightfieldRendererNode();
    
    void render(Heightfield* heightfield, const MetavoxelLOD& lod, const glm::vec2& minimum, float size,
        bool contained, bool cursor = false);
    
private:

    bool isLeaf() const;

    HeightfieldNodePointer _heightfieldNode;

    HeightfieldRendererNodePointer _children[CHILD_COUNT];
    
    GLuint _heightTextureID;
    GLuint _colorTextureID;
    GLuint _materialTextureID;
    QVector<NetworkTexturePointer> _networkTextures;
    
    typedef QPair<int, int> IntPair;    
    typedef QPair<QOpenGLBuffer, QOpenGLBuffer> BufferPair;
    static QHash<IntPair, BufferPair> _bufferPairs;
};

#endif // hifi_MetavoxelSystem_h
