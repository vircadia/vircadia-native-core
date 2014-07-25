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

class BufferPoint;
class Model;

typedef QVector<BufferPoint> BufferPointVector;
typedef QPair<BufferPointVector, BufferPointVector> BufferPointVectorPair;

/// Renders a metavoxel tree.
class MetavoxelSystem : public MetavoxelClientManager {
    Q_OBJECT

public:

    const AttributePointer& getPointBufferAttribute() const { return _pointBufferAttribute; }

    virtual void init();

    virtual MetavoxelLOD getLOD();
    
    void simulate(float deltaTime);
    void render();

    Q_INVOKABLE void setClientPoints(const SharedNodePointer& node, const BufferPointVector& points);

protected:

    virtual MetavoxelClient* createClient(const SharedNodePointer& node);

private:
    
    static ProgramObject _program;
    static int _pointScaleLocation;
    
    AttributePointer _pointBufferAttribute;
    
    MetavoxelLOD _lod;
    QReadWriteLock _lodLock;
};

/// Describes contents of a point in a point buffer.
class BufferPoint {
public:
    glm::vec4 vertex;
    quint8 color[3];
    quint8 normal[3];
};

Q_DECLARE_METATYPE(BufferPointVector)

/// A client session associated with a single server.
class MetavoxelSystemClient : public MetavoxelClient {
    Q_OBJECT    
    
public:
    
    MetavoxelSystemClient(const SharedNodePointer& node, MetavoxelUpdater* updater);
    
    void render();

    void setPoints(const BufferPointVector& points);
    
    virtual int parseData(const QByteArray& packet);

protected:
    
    virtual void dataChanged(const MetavoxelData& oldData);
    virtual void sendDatagram(const QByteArray& data);

private:
    
    QOpenGLBuffer _buffer;
    int _pointCount;
};

/// Contains the information necessary to render a group of points at variable detail levels.
class PointBuffer : public QSharedData {
public:

    PointBuffer(const QOpenGLBuffer& buffer, const QVector<int>& offsets, int lastLeafCount);

    void render(int level);

private:
    
    QOpenGLBuffer _buffer;
    QVector<int> _offsets;
    int _lastLeafCount;
};

typedef QExplicitlySharedDataPointer<PointBuffer> PointBufferPointer;

/// A client-side attribute that stores point buffers.
class PointBufferAttribute : public InlineAttribute<PointBufferPointer> {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE PointBufferAttribute();
    
    virtual MetavoxelNode* createMetavoxelNode(const AttributeValue& value, const MetavoxelNode* original) const;
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
    virtual AttributeValue inherit(const AttributeValue& parentValue) const;
};

/// Renders metavoxels as points.
class PointMetavoxelRendererImplementation : public MetavoxelRendererImplementation {
    Q_OBJECT

public:
    
    Q_INVOKABLE PointMetavoxelRendererImplementation();
    
    virtual void render(MetavoxelInfo& info);
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
