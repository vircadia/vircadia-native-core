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

class Model;

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
    
    const AttributePointer& getPointBufferAttribute() { return _pointBufferAttribute; }
    const AttributePointer& getHeightfieldBufferAttribute() { return _heightfieldBufferAttribute; }
    const AttributePointer& getVoxelBufferAttribute() { return _voxelBufferAttribute; }
    
    void simulate(float deltaTime);
    void render();

    void renderHeightfieldCursor(const glm::vec3& position, float radius);

    void renderVoxelCursor(const glm::vec3& position, float radius);

    bool findFirstRayHeightfieldIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance);

    bool findFirstRayVoxelIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance);

    Q_INVOKABLE float getHeightfieldHeight(const glm::vec3& location);

    Q_INVOKABLE void paintHeightfieldColor(const glm::vec3& position, float radius, const QColor& color);

    Q_INVOKABLE void paintHeightfieldMaterial(const glm::vec3& position, float radius, const SharedObjectPointer& material);
        
    Q_INVOKABLE void paintVoxelColor(const glm::vec3& position, float radius, const QColor& color);
    
    Q_INVOKABLE void paintVoxelMaterial(const glm::vec3& position, float radius, const SharedObjectPointer& material);
        
    Q_INVOKABLE void setVoxelColor(const SharedObjectPointer& spanner, const QColor& color);
        
    Q_INVOKABLE void setVoxelMaterial(const SharedObjectPointer& spanner, const SharedObjectPointer& material);
        
    Q_INVOKABLE void deleteTextures(int heightID, int colorID, int textureID);

signals:

    void rendering();

public slots:

    void refreshVoxelData();

protected:

    Q_INVOKABLE void applyMaterialEdit(const MetavoxelEditMessage& message, bool reliable = false);

    virtual MetavoxelClient* createClient(const SharedNodePointer& node);

private:
    
    void guideToAugmented(MetavoxelVisitor& visitor, bool render = false);
    
    AttributePointer _pointBufferAttribute;
    AttributePointer _heightfieldBufferAttribute;
    AttributePointer _voxelBufferAttribute;
    
    MetavoxelLOD _lod;
    QReadWriteLock _lodLock;
    Frustum _frustum;
    
    NetworkSimulation _networkSimulation;
    QReadWriteLock _networkSimulationLock;
};

/// Generic abstract base class for objects that handle a signal.
class SignalHandler : public QObject {
    Q_OBJECT
    
public slots:
    
    virtual void handle() = 0;
};

/// Describes contents of a point in a point buffer.
class BufferPoint {
public:
    glm::vec4 vertex;
    quint8 color[3];
    quint8 normal[3];
};

typedef QVector<BufferPoint> BufferPointVector;

Q_DECLARE_METATYPE(BufferPointVector)

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

/// Contains the information necessary to render a group of points.
class PointBuffer : public BufferData {
public:

    PointBuffer(const BufferPointVector& points);

    virtual void render(bool cursor = false);

private:
    
    BufferPointVector _points;
    QOpenGLBuffer _buffer;
    int _pointCount;
};

/// Contains the information necessary to render a heightfield block.
class HeightfieldBuffer : public BufferData {
public:
    
    static const int HEIGHT_BORDER;
    static const int SHARED_EDGE;
    static const int HEIGHT_EXTENSION;
    
    HeightfieldBuffer(const glm::vec3& translation, float scale, const QByteArray& height,
        const QByteArray& color, const QByteArray& material = QByteArray(),
        const QVector<SharedObjectPointer>& materials = QVector<SharedObjectPointer>());
    ~HeightfieldBuffer();
    
    const glm::vec3& getTranslation() const { return _translation; }
    float getScale() const { return _scale; }
    
    const Box& getHeightBounds() const { return _heightBounds; }
    const Box& getColorBounds() const { return _colorBounds; }
    
    QByteArray& getHeight() { return _height; }
    const QByteArray& getHeight() const { return _height; }
    
    QByteArray& getColor() { return _color; }
    const QByteArray& getColor() const { return _color; }
    
    QByteArray& getMaterial() { return _material; }
    const QByteArray& getMaterial() const { return _material; }
    
    const QVector<SharedObjectPointer>& getMaterials() const { return _materials; }
    
    QByteArray getUnextendedHeight() const;
    QByteArray getUnextendedColor() const;
    
    int getHeightSize() const { return _heightSize; }
    float getHeightIncrement() const { return _heightIncrement; }
    
    int getColorSize() const { return _colorSize; }
    float getColorIncrement() const { return _colorIncrement; }
    
    virtual void render(bool cursor = false);

private:
    
    glm::vec3 _translation;
    float _scale;
    Box _heightBounds;
    Box _colorBounds;
    QByteArray _height;
    QByteArray _color;
    QByteArray _material;
    QVector<SharedObjectPointer> _materials;
    GLuint _heightTextureID;
    GLuint _colorTextureID;
    GLuint _materialTextureID;
    QVector<NetworkTexturePointer> _networkTextures;
    int _heightSize;
    float _heightIncrement;
    int _colorSize;
    float _colorIncrement;
    
    typedef QPair<QOpenGLBuffer, QOpenGLBuffer> BufferPair;    
    static QHash<int, BufferPair> _bufferPairs;
};

/// Convenience class for rendering a preview of a heightfield.
class HeightfieldPreview {
public:
    
    void setBuffers(const QVector<BufferDataPointer>& buffers) { _buffers = buffers; }
    const QVector<BufferDataPointer>& getBuffers() const { return _buffers; }
    
    void render(const glm::vec3& translation, float scale) const;
    
private:
    
    QVector<BufferDataPointer> _buffers;
};

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

/// Contains the information necessary to render a voxel block.
class VoxelBuffer : public BufferData {
public:
    
    VoxelBuffer(const QVector<VoxelPoint>& vertices, const QVector<int>& indices, const QVector<glm::vec3>& hermite,
        const QMultiHash<QRgb, int>& quadIndices, int size, const QVector<SharedObjectPointer>& materials =
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
    QMultiHash<QRgb, int> _quadIndices;
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
    
    static void init();

    static ProgramObject& getPointProgram() { return _pointProgram; }
    static int getPointScaleLocation() { return _pointScaleLocation; }
    
    static ProgramObject& getBaseHeightfieldProgram() { return _baseHeightfieldProgram; }
    static int getBaseHeightScaleLocation() { return _baseHeightScaleLocation; }
    static int getBaseColorScaleLocation() { return _baseColorScaleLocation; }
    
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
    
    static ProgramObject& getSplatHeightfieldProgram() { return _splatHeightfieldProgram; }
    static const SplatLocations& getSplatHeightfieldLocations() { return _splatHeightfieldLocations; }
    
    static ProgramObject& getHeightfieldCursorProgram() { return _heightfieldCursorProgram; }
    
    static ProgramObject& getBaseVoxelProgram() { return _baseVoxelProgram; }
    
    static ProgramObject& getSplatVoxelProgram() { return _splatVoxelProgram; }
    static const SplatLocations& getSplatVoxelLocations() { return _splatVoxelLocations; }
    
    static ProgramObject& getVoxelCursorProgram() { return _voxelCursorProgram; }
    
    Q_INVOKABLE DefaultMetavoxelRendererImplementation();
    
    virtual void augment(MetavoxelData& data, const MetavoxelData& previous, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void simulate(MetavoxelData& data, float deltaTime, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod);

private:

    static void loadSplatProgram(const char* type, ProgramObject& program, SplatLocations& locations);
    
    static ProgramObject _pointProgram;
    static int _pointScaleLocation;
    
    static ProgramObject _baseHeightfieldProgram;
    static int _baseHeightScaleLocation;
    static int _baseColorScaleLocation;
    
    static ProgramObject _splatHeightfieldProgram;
    static SplatLocations _splatHeightfieldLocations;
    
    static int _splatHeightScaleLocation;
    static int _splatTextureScaleLocation;
    static int _splatTextureOffsetLocation;
    static int _splatTextureScalesSLocation;
    static int _splatTextureScalesTLocation;
    static int _splatTextureValueMinimaLocation;
    static int _splatTextureValueMaximaLocation;
    
    static ProgramObject _heightfieldCursorProgram;
    
    static ProgramObject _baseVoxelProgram;
    static ProgramObject _splatVoxelProgram;
    static SplatLocations _splatVoxelLocations;
    
    static ProgramObject _voxelCursorProgram;
};

/// Base class for spanner renderers; provides clipping.
class ClippedRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    virtual void render(const glm::vec4& color, Mode mode, const glm::vec3& clipMinimum, float clipSize);
    
protected:

    virtual void renderUnclipped(const glm::vec4& color, Mode mode) = 0;
};

/// Renders spheres.
class SphereRenderer : public ClippedRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE SphereRenderer();
    
    virtual void render(const glm::vec4& color, Mode mode, const glm::vec3& clipMinimum, float clipSize);
    
protected:

    virtual void renderUnclipped(const glm::vec4& color, Mode mode);
};

/// Renders cuboids.
class CuboidRenderer : public ClippedRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE CuboidRenderer();
    
protected:

    virtual void renderUnclipped(const glm::vec4& color, Mode mode);
};

/// Renders static models.
class StaticModelRenderer : public ClippedRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE StaticModelRenderer();
    
    virtual void init(Spanner* spanner);
    virtual void simulate(float deltaTime);
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& clipMinimum, float clipSize, float& distance) const;

protected:

    virtual void renderUnclipped(const glm::vec4& color, Mode mode);

private slots:

    void applyTranslation(const glm::vec3& translation);
    void applyRotation(const glm::quat& rotation);
    void applyScale(float scale);
    void applyURL(const QUrl& url);

private:
    
    Model* _model;
};

#endif // hifi_MetavoxelSystem_h
