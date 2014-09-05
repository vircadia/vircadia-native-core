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

    virtual void init();

    virtual MetavoxelLOD getLOD();

    const Frustum& getFrustum() const { return _frustum; }
    
    const AttributePointer& getPointBufferAttribute() { return _pointBufferAttribute; }
    const AttributePointer& getHeightfieldBufferAttribute() { return _heightfieldBufferAttribute; }
    const AttributePointer& getVoxelBufferAttribute() { return _voxelBufferAttribute; }
    
    void simulate(float deltaTime);
    void render();

    void renderHeightfieldCursor(const glm::vec3& position, float radius);

    bool findFirstRayHeightfieldIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance);

    Q_INVOKABLE float getHeightfieldHeight(const glm::vec3& location);

    Q_INVOKABLE void deleteTextures(int heightID, int colorID, int textureID);

    void noteNeedToLight() { _needToLight = true; }

signals:

    void rendering();

protected:

    virtual MetavoxelClient* createClient(const SharedNodePointer& node);

private:
    
    void guideToAugmented(MetavoxelVisitor& visitor, bool render = false);
    
    class LightLocations {
    public:
        int shadowDistances;
        int shadowScale;
        int near;
        int depthScale;
        int depthTexCoordOffset;
        int depthTexCoordScale;
    };
    
    static void loadLightProgram(const char* name, ProgramObject& program, LightLocations& locations);
    
    AttributePointer _pointBufferAttribute;
    AttributePointer _heightfieldBufferAttribute;
    AttributePointer _voxelBufferAttribute;
    
    MetavoxelLOD _lod;
    QReadWriteLock _lodLock;
    Frustum _frustum;
    bool _needToLight;
    
    ProgramObject _directionalLight;
    LightLocations _directionalLightLocations;
    ProgramObject _directionalLightShadowMap;
    LightLocations _directionalLightShadowMapLocations;
    ProgramObject _directionalLightCascadedShadowMap;
    LightLocations _directionalLightCascadedShadowMapLocations;
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

protected:
    
    virtual void dataChanged(const MetavoxelData& oldData);
    virtual void sendDatagram(const QByteArray& data);

private:
    
    MetavoxelData _augmentedData;
    MetavoxelData _renderedAugmentedData;
    QReadWriteLock _augmentedDataLock;
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
};

/// Contains the information necessary to render a voxel block.
class VoxelBuffer : public BufferData {
public:
    
    VoxelBuffer(const QVector<VoxelPoint>& vertices, const QVector<int>& indices,
        const QVector<SharedObjectPointer>& materials = QVector<SharedObjectPointer>());
        
    virtual void render(bool cursor = false);

private:
    
    QVector<VoxelPoint> _vertices;
    QVector<int> _indices;
    int _vertexCount;
    int _indexCount;
    QOpenGLBuffer _vertexBuffer;
    QOpenGLBuffer _indexBuffer;
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
};

/// Base class for spanner renderers; provides clipping.
class ClippedRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    virtual void render(float alpha, Mode mode, const glm::vec3& clipMinimum, float clipSize);
    
protected:

    virtual void renderUnclipped(float alpha, Mode mode) = 0;
};

/// Renders spheres.
class SphereRenderer : public ClippedRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE SphereRenderer();
    
    virtual void render(float alpha, Mode mode, const glm::vec3& clipMinimum, float clipSize);
    
protected:

    virtual void renderUnclipped(float alpha, Mode mode);
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

    virtual void renderUnclipped(float alpha, Mode mode);

private slots:

    void applyTranslation(const glm::vec3& translation);
    void applyRotation(const glm::quat& rotation);
    void applyScale(float scale);
    void applyURL(const QUrl& url);

private:
    
    Model* _model;
};

#endif // hifi_MetavoxelSystem_h
