//
//  GeometryCache.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GeometryCache_h
#define hifi_GeometryCache_h

#include "model-networking/ModelCache.h"

#include <array>


#include <QMap>
#include <QRunnable>

#include <DependencyManager.h>

#include <gpu/Batch.h>
#include <gpu/Stream.h>


#include <model/Material.h>
#include <model/Asset.h>

typedef glm::vec3 Vec3Key;

typedef QPair<glm::vec2, glm::vec2> Vec2Pair;
typedef QPair<Vec2Pair, Vec2Pair> Vec2PairPair;
typedef QPair<glm::vec3, glm::vec3> Vec3Pair;
typedef QPair<glm::vec4, glm::vec4> Vec4Pair;
typedef QPair<Vec3Pair, Vec2Pair> Vec3PairVec2Pair;
typedef QPair<Vec3Pair, glm::vec4> Vec3PairVec4;
typedef QPair<Vec3Pair, Vec4Pair> Vec3PairVec4Pair;
typedef QPair<Vec4Pair, glm::vec4> Vec4PairVec4;
typedef QPair<Vec4Pair, Vec4Pair> Vec4PairVec4Pair;

inline uint qHash(const glm::vec2& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.x + 5009 * v.y, seed);
}

inline uint qHash(const Vec2Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.x + 5009 * v.first.y + 5011 * v.second.x + 5021 * v.second.y, seed);
}

inline uint qHash(const glm::vec4& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.x + 5009 * v.y + 5011 * v.z + 5021 * v.w, seed);
}

inline uint qHash(const Vec2PairPair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y 
                 + 5011 * v.first.second.x + 5021 * v.first.second.y
                 + 5023 * v.second.first.x + 5039 * v.second.first.y
                 + 5051 * v.second.second.x + 5059 * v.second.second.y, seed);
}

inline uint qHash(const Vec3Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.x + 5009 * v.first.y + 5011 * v.first.z 
                 + 5021 * v.second.x + 5023 * v.second.y + 5039 * v.second.z, seed);
}

inline uint qHash(const Vec4Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.x + 5009 * v.first.y + 5011 * v.first.z + 5021 * v.first.w 
                    + 5023 * v.second.x + 5039 * v.second.y + 5051 * v.second.z + 5059 * v.second.w , seed);
}

inline uint qHash(const Vec3PairVec2Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z +
                 5021 * v.first.second.x + 5023 * v.first.second.y + 5039 * v.first.second.z +
                 5051 * v.second.first.x + 5059 * v.second.first.y +
                 5077 * v.second.second.x + 5081 * v.second.second.y, seed);
}

inline uint qHash(const Vec3PairVec4& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z +
                 5021 * v.first.second.x + 5023 * v.first.second.y + 5039 * v.first.second.z +
                 5051 * v.second.x + 5059 * v.second.y + 5077 * v.second.z + 5081 * v.second.w, seed);
}


inline uint qHash(const Vec3PairVec4Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z 
                + 5023 * v.first.second.x + 5039 * v.first.second.y + 5051 * v.first.second.z 
                + 5077 * v.second.first.x + 5081 * v.second.first.y + 5087 * v.second.first.z + 5099 * v.second.first.w 
                + 5101 * v.second.second.x + 5107 * v.second.second.y + 5113 * v.second.second.z + 5119 * v.second.second.w, 
                seed);
}

inline uint qHash(const Vec4PairVec4& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z + 5021 * v.first.first.w 
                + 5023 * v.first.second.x + 5039 * v.first.second.y + 5051 * v.first.second.z + 5059 * v.first.second.w 
                + 5077 * v.second.x + 5081 * v.second.y + 5087 * v.second.z + 5099 * v.second.w,
                seed);
}

inline uint qHash(const Vec4PairVec4Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z + 5021 * v.first.first.w 
                + 5023 * v.first.second.x + 5039 * v.first.second.y + 5051 * v.first.second.z + 5059 * v.first.second.w 
                + 5077 * v.second.first.x + 5081 * v.second.first.y + 5087 * v.second.first.z + 5099 * v.second.first.w 
                + 5101 * v.second.second.x + 5107 * v.second.second.y + 5113 * v.second.second.z + 5119 * v.second.second.w, 
                seed);
}

using VertexVector = std::vector<glm::vec3>;
using IndexVector = std::vector<uint16_t>;

/// Stores cached geometry.
class GeometryCache : public Dependency {
    SINGLETON_DEPENDENCY

public:
    enum Shape {
        Line,
        Triangle,
        Quad,
        Circle,
        Cube,
        Sphere,
        Tetrahedron,
        Octahetron,
        Dodecahedron,
        Icosahedron,
        Torus,
        Cone,
        Cylinder,

        NUM_SHAPES,
    };

    int allocateID() { return _nextID++; }
    static const int UNKNOWN_ID;

    void renderShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& transformBuffer, gpu::BufferPointer& colorBuffer);
    void renderWireShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& transformBuffer, gpu::BufferPointer& colorBuffer);
    void renderShape(gpu::Batch& batch, Shape shape);
    void renderWireShape(gpu::Batch& batch, Shape shape);

    void renderCubeInstances(gpu::Batch& batch, size_t count, gpu::BufferPointer transformBuffer, gpu::BufferPointer colorBuffer);
    void renderWireCubeInstances(gpu::Batch& batch, size_t count, gpu::BufferPointer transformBuffer, gpu::BufferPointer colorBuffer);
    void renderCube(gpu::Batch& batch);
    void renderWireCube(gpu::Batch& batch);

    void renderSphereInstances(gpu::Batch& batch, size_t count, gpu::BufferPointer transformBuffer, gpu::BufferPointer colorBuffer);
    void renderWireSphereInstances(gpu::Batch& batch, size_t count, gpu::BufferPointer transformBuffer, gpu::BufferPointer colorBuffer);
    void renderSphere(gpu::Batch& batch);
    void renderWireSphere(gpu::Batch& batch);

    void renderGrid(gpu::Batch& batch, int xDivisions, int yDivisions, const glm::vec4& color);
    void renderGrid(gpu::Batch& batch, int x, int y, int width, int height, int rows, int cols, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderBevelCornersRect(gpu::Batch& batch, int x, int y, int width, int height, int bevelDistance, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderUnitQuad(gpu::Batch& batch, const glm::vec4& color = glm::vec4(1), int id = UNKNOWN_ID);

    void renderQuad(gpu::Batch& batch, int x, int y, int width, int height, const glm::vec4& color, int id = UNKNOWN_ID)
            { renderQuad(batch, glm::vec2(x,y), glm::vec2(x + width, y + height), color, id); }
            
    // TODO: I think there's a bug in this version of the renderQuad() that's not correctly rebuilding the vbos
    // if the color changes by the corners are the same, as evidenced by the audio meter which should turn white
    // when it's clipping
    void renderQuad(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderQuad(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner,
                    const glm::vec2& texCoordMinCorner, const glm::vec2& texCoordMaxCorner, 
                    const glm::vec4& color, int id = UNKNOWN_ID);

    void renderQuad(gpu::Batch& batch, const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderQuad(gpu::Batch& batch, const glm::vec3& topLeft, const glm::vec3& bottomLeft, 
                    const glm::vec3& bottomRight, const glm::vec3& topRight,
                    const glm::vec2& texCoordTopLeft, const glm::vec2& texCoordBottomLeft,
                    const glm::vec2& texCoordBottomRight, const glm::vec2& texCoordTopRight, 
                    const glm::vec4& color, int id = UNKNOWN_ID);


    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color, int id = UNKNOWN_ID) 
                    { renderLine(batch, p1, p2, color, color, id); }
    
    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec3& color1, const glm::vec3& color2, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, glm::vec4(color1, 1.0f), glm::vec4(color2, 1.0f), id); }

    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, color, color, id); }

    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color1, const glm::vec4& color2, int id = UNKNOWN_ID);

    void renderDashedLine(gpu::Batch& batch, const glm::vec3& start, const glm::vec3& end, const glm::vec4& color,
                          int id = UNKNOWN_ID)
                          { renderDashedLine(batch, start, end, color, 0.05f, 0.025f, id); }

    void renderDashedLine(gpu::Batch& batch, const glm::vec3& start, const glm::vec3& end, const glm::vec4& color,
                          const float dash_length, const float gap_length, int id = UNKNOWN_ID);

    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2, const glm::vec3& color, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, glm::vec4(color, 1.0f), id); }

    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, color, color, id); }

    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2,
                    const glm::vec3& color1, const glm::vec3& color2, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, glm::vec4(color1, 1.0f), glm::vec4(color2, 1.0f), id); }

    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2,
                                    const glm::vec4& color1, const glm::vec4& color2, int id = UNKNOWN_ID);

    void updateVertices(int id, const QVector<glm::vec2>& points, const glm::vec4& color);
    void updateVertices(int id, const QVector<glm::vec3>& points, const glm::vec4& color);
    void updateVertices(int id, const QVector<glm::vec3>& points, const QVector<glm::vec2>& texCoords, const glm::vec4& color);
    void renderVertices(gpu::Batch& batch, gpu::Primitive primitiveType, int id);

    /// Set a batch to the simple pipeline, returning the previous pipeline
    void useSimpleDrawPipeline(gpu::Batch& batch, bool noBlend = false);

private:
    GeometryCache();
    virtual ~GeometryCache();
    void buildShapes();

    typedef QPair<int, int> IntPair;
    typedef QPair<unsigned int, unsigned int> VerticesIndices;

    struct ShapeData {
        size_t _indexOffset{ 0 };
        size_t _indexCount{ 0 };
        size_t _wireIndexOffset{ 0 };
        size_t _wireIndexCount{ 0 };

        gpu::BufferView _positionView;
        gpu::BufferView _normalView;
        gpu::BufferPointer _indices;

        void setupVertices(gpu::BufferPointer& vertexBuffer, const VertexVector& vertices);
        void setupIndices(gpu::BufferPointer& indexBuffer, const IndexVector& indices, const IndexVector& wireIndices);
        void setupBatch(gpu::Batch& batch) const;
        void draw(gpu::Batch& batch) const;
        void drawWire(gpu::Batch& batch) const;
        void drawInstances(gpu::Batch& batch, size_t count) const;
        void drawWireInstances(gpu::Batch& batch, size_t count) const;
    };

    using VShape = std::array<ShapeData, NUM_SHAPES>;

    VShape _shapes;



    gpu::PipelinePointer _standardDrawPipeline;
    gpu::PipelinePointer _standardDrawPipelineNoBlend;

    gpu::BufferPointer _shapeVertices{ std::make_shared<gpu::Buffer>() };
    gpu::BufferPointer _shapeIndices{ std::make_shared<gpu::Buffer>() };

    class BatchItemDetails {
    public:
        static int population;
        gpu::BufferPointer verticesBuffer;
        gpu::BufferPointer colorBuffer;
        gpu::Stream::FormatPointer streamFormat;
        gpu::BufferStreamPointer stream;

        int vertices;
        int vertexSize;
        bool isCreated;
        
        BatchItemDetails();
        BatchItemDetails(const GeometryCache::BatchItemDetails& other);
        ~BatchItemDetails();
        void clear();
    };

    QHash<IntPair, VerticesIndices> _coneVBOs;

    int _nextID{ 0 };

    QHash<int, Vec3PairVec4Pair> _lastRegisteredQuad3DTexture;
    QHash<Vec3PairVec4Pair, BatchItemDetails> _quad3DTextures;
    QHash<int, BatchItemDetails> _registeredQuad3DTextures;

    QHash<int, Vec4PairVec4> _lastRegisteredQuad2DTexture;
    QHash<Vec4PairVec4, BatchItemDetails> _quad2DTextures;
    QHash<int, BatchItemDetails> _registeredQuad2DTextures;

    QHash<int, Vec3PairVec4> _lastRegisteredQuad3D;
    QHash<Vec3PairVec4, BatchItemDetails> _quad3D;
    QHash<int, BatchItemDetails> _registeredQuad3D;

    QHash<int, Vec4Pair> _lastRegisteredQuad2D;
    QHash<Vec4Pair, BatchItemDetails> _quad2D;
    QHash<int, BatchItemDetails> _registeredQuad2D;

    QHash<int, Vec3Pair> _lastRegisteredBevelRects;
    QHash<Vec3Pair, BatchItemDetails> _bevelRects;
    QHash<int, BatchItemDetails> _registeredBevelRects;

    QHash<int, Vec3Pair> _lastRegisteredLine3D;
    QHash<Vec3Pair, BatchItemDetails> _line3DVBOs;
    QHash<int, BatchItemDetails> _registeredLine3DVBOs;

    QHash<int, Vec2Pair> _lastRegisteredLine2D;
    QHash<Vec2Pair, BatchItemDetails> _line2DVBOs;
    QHash<int, BatchItemDetails> _registeredLine2DVBOs;
    
    QHash<int, BatchItemDetails> _registeredVertices;

    QHash<int, Vec3PairVec2Pair> _lastRegisteredDashedLines;
    QHash<Vec3PairVec2Pair, BatchItemDetails> _dashedLines;
    QHash<int, BatchItemDetails> _registeredDashedLines;

    QHash<IntPair, gpu::BufferPointer> _gridBuffers;
    QHash<Vec3Pair, gpu::BufferPointer> _alternateGridBuffers;
    QHash<int, gpu::BufferPointer> _registeredAlternateGridBuffers;
    QHash<int, Vec3Pair> _lastRegisteredAlternateGridBuffers;
    QHash<Vec3Pair, gpu::BufferPointer> _gridColors;

    QHash<QUrl, QWeakPointer<NetworkGeometry> > _networkGeometry;
};

#endif // hifi_GeometryCache_h
