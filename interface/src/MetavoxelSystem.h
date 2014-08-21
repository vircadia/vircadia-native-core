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
    
    void simulate(float deltaTime);
    void render();

    void renderHeightfieldCursor(const glm::vec3& position, float radius);

    bool findFirstRayHeightfieldIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance);

    Q_INVOKABLE float getHeightfieldHeight(const glm::vec3& location);

    Q_INVOKABLE void deleteTextures(int heightID, int colorID, int textureID);

protected:

    virtual MetavoxelClient* createClient(const SharedNodePointer& node);

private:
    
    void guideToAugmented(MetavoxelVisitor& visitor);
    
    AttributePointer _pointBufferAttribute;
    AttributePointer _heightfieldBufferAttribute;
    
    MetavoxelLOD _lod;
    QReadWriteLock _lodLock;
    Frustum _frustum;
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
    
    virtual int parseData(const QByteArray& packet);

protected:
    
    virtual void dataChanged(const MetavoxelData& oldData);
    virtual void sendDatagram(const QByteArray& data);

private:
    
    MetavoxelData _augmentedData;
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
        const QByteArray& color, const QByteArray& texture = QByteArray(),
        const QVector<SharedObjectPointer>& textures = QVector<SharedObjectPointer>());
    ~HeightfieldBuffer();
    
    const glm::vec3& getTranslation() const { return _translation; }
    float getScale() const { return _scale; }
    
    const Box& getHeightBounds() const { return _heightBounds; }
    const Box& getColorBounds() const { return _colorBounds; }
    
    QByteArray& getHeight() { return _height; }
    const QByteArray& getHeight() const { return _height; }
    
    QByteArray& getColor() { return _color; }
    const QByteArray& getColor() const { return _color; }
    
    QByteArray& getTexture() { return _texture; }
    const QByteArray& getTexture() const { return _texture; }
    
    const QVector<SharedObjectPointer>& getTextures() const { return _textures; }
    
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
    QByteArray _texture;
    QVector<SharedObjectPointer> _textures;
    GLuint _heightTextureID;
    GLuint _colorTextureID;
    GLuint _textureTextureID;
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

    static ProgramObject& getHeightfieldProgram() { return _heightfieldProgram; }
    static int getHeightScaleLocation() { return _heightScaleLocation; }
    static int getColorScaleLocation() { return _colorScaleLocation; }
    
    static ProgramObject& getShadowMapHeightfieldProgram() { return _shadowMapHeightfieldProgram; }
    static int getShadowMapHeightScaleLocation() { return _shadowMapHeightScaleLocation; }
    static int getShadowMapColorScaleLocation() { return _shadowMapColorScaleLocation; }
    
    static ProgramObject& getCascadedShadowMapHeightfieldProgram() { return _cascadedShadowMapHeightfieldProgram; }
    static int getCascadedShadowMapHeightScaleLocation() { return _cascadedShadowMapHeightScaleLocation; }
    static int getCascadedShadowMapColorScaleLocation() { return _cascadedShadowMapColorScaleLocation; }
    
    static ProgramObject& getBaseHeightfieldProgram() { return _baseHeightfieldProgram; }
    static int getBaseHeightScaleLocation() { return _baseHeightScaleLocation; }
    static int getBaseColorScaleLocation() { return _baseColorScaleLocation; }
    
    static ProgramObject& getSplatHeightfieldProgram() { return _splatHeightfieldProgram; }
    static int getSplatHeightScaleLocation() { return _splatHeightScaleLocation; }
    static int getSplatTextureScaleLocation() { return _splatTextureScaleLocation; }
    static int getSplatTextureValueMinimaLocation() { return _splatTextureValueMinimaLocation; }
    static int getSplatTextureValueMaximaLocation() { return _splatTextureValueMaximaLocation; }
    
    static ProgramObject& getLightHeightfieldProgram() { return _lightHeightfieldProgram; }
    static int getLightHeightScaleLocation() { return _lightHeightScaleLocation; }
    
    static ProgramObject& getShadowLightHeightfieldProgram() { return _shadowLightHeightfieldProgram; }
    static int getShadowLightHeightScaleLocation() { return _shadowLightHeightScaleLocation; }
    
    static ProgramObject& getCascadedShadowLightHeightfieldProgram() { return _cascadedShadowLightHeightfieldProgram; }
    static int getCascadedShadowLightHeightScaleLocation() { return _cascadedShadowLightHeightScaleLocation; }
    
    static ProgramObject& getHeightfieldCursorProgram() { return _heightfieldCursorProgram; }
    
    Q_INVOKABLE DefaultMetavoxelRendererImplementation();
    
    virtual void augment(MetavoxelData& data, const MetavoxelData& previous, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void simulate(MetavoxelData& data, float deltaTime, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod);

private:

    static ProgramObject _pointProgram;
    static int _pointScaleLocation;
    
    static ProgramObject _heightfieldProgram;
    static int _heightScaleLocation;
    static int _colorScaleLocation;
    
    static ProgramObject _shadowMapHeightfieldProgram;
    static int _shadowMapHeightScaleLocation;
    static int _shadowMapColorScaleLocation;
    
    static ProgramObject _cascadedShadowMapHeightfieldProgram;
    static int _cascadedShadowMapHeightScaleLocation;
    static int _cascadedShadowMapColorScaleLocation;
    static int _shadowDistancesLocation;
    
    static ProgramObject _baseHeightfieldProgram;
    static int _baseHeightScaleLocation;
    static int _baseColorScaleLocation;
    
    static ProgramObject _splatHeightfieldProgram;
    static int _splatHeightScaleLocation;
    static int _splatTextureScaleLocation;
    static int _splatTextureValueMinimaLocation;
    static int _splatTextureValueMaximaLocation;
    
    static ProgramObject _lightHeightfieldProgram;
    static int _lightHeightScaleLocation;
    
    static ProgramObject _shadowLightHeightfieldProgram;
    static int _shadowLightHeightScaleLocation;
    
    static ProgramObject _cascadedShadowLightHeightfieldProgram;
    static int _cascadedShadowLightHeightScaleLocation;
    static int _shadowLightDistancesLocation;
    
    static ProgramObject _heightfieldCursorProgram;
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
