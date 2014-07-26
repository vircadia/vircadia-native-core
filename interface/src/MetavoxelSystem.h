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

    virtual void init();

    virtual MetavoxelLOD getLOD();
    
    const AttributePointer& getPointBufferAttribute() { return _pointBufferAttribute; }
    
    void simulate(float deltaTime);
    void render();

protected:

    virtual MetavoxelClient* createClient(const SharedNodePointer& node);

private:
    
    void guideToAugmented(MetavoxelVisitor& visitor);
    
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

/// Contains the information necessary to render a group of points at variable detail levels.
class PointBuffer : public QSharedData {
public:

    PointBuffer(const QVector<BufferPointVectorPair>& levelPoints);

    void render(int level);

private:
    
    QVector<BufferPointVectorPair> _levelPoints;
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
    
    static void init();
    
    Q_INVOKABLE PointMetavoxelRendererImplementation();
    
    virtual void augment(MetavoxelData& data, const MetavoxelData& previous, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod);

private:

    static ProgramObject _program;
    static int _pointScaleLocation;    
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
