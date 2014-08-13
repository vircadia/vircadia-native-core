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

    Q_INVOKABLE void deleteTextures(int heightID, int colorID);

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
    
    static const int HEIGHT_BORDER = 1;
    static const int SHARED_EDGE = 1;
    static const int HEIGHT_EXTENSION = 2 * HEIGHT_BORDER + SHARED_EDGE;
    
    HeightfieldBuffer(const glm::vec3& translation, float scale, const QByteArray& height, const QByteArray& color);
    ~HeightfieldBuffer();
    
    const glm::vec3& getTranslation() const { return _translation; }
    float getScale() const { return _scale; }
    
    QByteArray& getHeight() { return _height; }
    QByteArray& getColor() { return _color; }
    
    QByteArray getUnextendedHeight() const;
    QByteArray getUnextendedColor() const;
    
    virtual void render(bool cursor = false);

private:
    
    glm::vec3 _translation;
    float _scale;
    QByteArray _height;
    QByteArray _color;
    GLuint _heightTextureID;
    GLuint _colorTextureID;
    int _heightSize;

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
};

/// Renders metavoxels as points.
class DefaultMetavoxelRendererImplementation : public MetavoxelRendererImplementation {
    Q_OBJECT

public:
    
    static void init();

    static ProgramObject& getHeightfieldProgram() { return _heightfieldProgram; }
    static int getHeightScaleLocation() { return _heightScaleLocation; }
    static int getColorScaleLocation() { return _colorScaleLocation; }
    
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
