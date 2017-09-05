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

#include <shared/Shapes.h>

#include <gpu/Batch.h>
#include <gpu/Stream.h>

#include <render/ShapePipeline.h>

#include <model/Material.h>
#include <model/Asset.h>

class SimpleProgramKey;

typedef QPair<glm::vec2, float> Vec2FloatPair;
typedef QPair<Vec2FloatPair, Vec2FloatPair> Vec2FloatPairPair;
typedef QPair<glm::vec2, glm::vec2> Vec2Pair;
typedef QPair<Vec2Pair, Vec2Pair> Vec2PairPair;
typedef QPair<glm::vec3, glm::vec3> Vec3Pair;
typedef QPair<glm::vec4, glm::vec4> Vec4Pair;
typedef QPair<Vec3Pair, Vec2Pair> Vec3PairVec2Pair;
typedef QPair<Vec3Pair, glm::vec4> Vec3PairVec4;
typedef QPair<Vec3Pair, Vec4Pair> Vec3PairVec4Pair;
typedef QPair<Vec4Pair, glm::vec4> Vec4PairVec4;
typedef QPair<Vec4Pair, Vec4Pair> Vec4PairVec4Pair;

inline uint qHash(const Vec2FloatPairPair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.second +
        5021 * v.second.first.x + 5023 * v.second.first.y + 5039 * v.second.second);
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

/// Stores cached geometry.
class GeometryCache : public Dependency {
    SINGLETON_DEPENDENCY

public:
    enum Shape {
        Line,
        Triangle,
        Quad,
        Hexagon,
        Octagon,
        Circle,
        Cube,
        Sphere,
        Tetrahedron,
        Octahedron,
        Dodecahedron,
        Icosahedron,
        Torus, // not yet implemented
        Cone, 
        Cylinder, 
        NUM_SHAPES,
    };

    static uint8_t CUSTOM_PIPELINE_NUMBER;
    static render::ShapePipelinePointer shapePipelineFactory(const render::ShapePlumber& plumber, const render::ShapeKey& key);
    static void registerShapePipeline() {
        if (!CUSTOM_PIPELINE_NUMBER) {
            CUSTOM_PIPELINE_NUMBER = render::ShapePipeline::registerCustomShapePipelineFactory(shapePipelineFactory);
        }
    }

    int allocateID() { return _nextID++; }
    void releaseID(int id);
    static const int UNKNOWN_ID;

    // Bind the pipeline and get the state to render static geometry
    void bindSimpleProgram(gpu::Batch& batch, bool textured = false, bool transparent = false, bool culled = true,
                                          bool unlit = false, bool depthBias = false);
    // Get the pipeline to render static geometry
    static gpu::PipelinePointer getSimplePipeline(bool textured = false, bool transparent = false, bool culled = true,
                                          bool unlit = false, bool depthBias = false, bool fading = false);

    void bindWebBrowserProgram(gpu::Batch& batch, bool transparent = false);
    gpu::PipelinePointer getWebBrowserProgram(bool transparent);

    static void initializeShapePipelines();

    render::ShapePipelinePointer getOpaqueShapePipeline() { assert(_simpleOpaquePipeline != nullptr); return _simpleOpaquePipeline; }
    render::ShapePipelinePointer getTransparentShapePipeline() { assert(_simpleTransparentPipeline != nullptr); return _simpleTransparentPipeline; }
    render::ShapePipelinePointer getOpaqueFadeShapePipeline() { assert(_simpleOpaqueFadePipeline != nullptr); return _simpleOpaqueFadePipeline; }
    render::ShapePipelinePointer getTransparentFadeShapePipeline() { assert(_simpleTransparentFadePipeline != nullptr); return _simpleTransparentFadePipeline; }
    render::ShapePipelinePointer getOpaqueShapePipeline(bool isFading);
    render::ShapePipelinePointer getTransparentShapePipeline(bool isFading);
    render::ShapePipelinePointer getWireShapePipeline() { assert(_simpleWirePipeline != nullptr); return GeometryCache::_simpleWirePipeline; }


    // Static (instanced) geometry
    void renderShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& colorBuffer);
    void renderWireShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& colorBuffer);

    void renderFadeShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& colorBuffer,
        gpu::BufferPointer& fadeBuffer1, gpu::BufferPointer& fadeBuffer2, gpu::BufferPointer& fadeBuffer3);
    void renderWireFadeShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& colorBuffer,
        gpu::BufferPointer& fadeBuffer1, gpu::BufferPointer& fadeBuffer2, gpu::BufferPointer& fadeBuffer3);

    void renderSolidShapeInstance(RenderArgs* args, gpu::Batch& batch, Shape shape, const glm::vec4& color = glm::vec4(1),
                                    const render::ShapePipelinePointer& pipeline = _simpleOpaquePipeline);
    void renderSolidShapeInstance(RenderArgs* args, gpu::Batch& batch, Shape shape, const glm::vec3& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleOpaquePipeline) {
        renderSolidShapeInstance(args, batch, shape, glm::vec4(color, 1.0f), pipeline);
    }

    void renderWireShapeInstance(RenderArgs* args, gpu::Batch& batch, Shape shape, const glm::vec4& color = glm::vec4(1),
        const render::ShapePipelinePointer& pipeline = _simpleOpaquePipeline);
    void renderWireShapeInstance(RenderArgs* args, gpu::Batch& batch, Shape shape, const glm::vec3& color,
        const render::ShapePipelinePointer& pipeline = _simpleOpaquePipeline) {
        renderWireShapeInstance(args, batch, shape, glm::vec4(color, 1.0f), pipeline);
    }

    void renderSolidFadeShapeInstance(RenderArgs* args, gpu::Batch& batch, Shape shape, const glm::vec4& color, int fadeCategory, float fadeThreshold,
        const glm::vec3& fadeNoiseOffset, const glm::vec3& fadeBaseOffset, const glm::vec3& fadeBaseInvSize,
        const render::ShapePipelinePointer& pipeline);
    void renderWireFadeShapeInstance(RenderArgs* args, gpu::Batch& batch, Shape shape, const glm::vec4& color, int fadeCategory, float fadeThreshold,
        const glm::vec3& fadeNoiseOffset, const glm::vec3& fadeBaseOffset, const glm::vec3& fadeBaseInvSize,
        const render::ShapePipelinePointer& pipeline);

    void renderSolidSphereInstance(RenderArgs* args, gpu::Batch& batch, const glm::vec4& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleOpaquePipeline);
    void renderSolidSphereInstance(RenderArgs* args, gpu::Batch& batch, const glm::vec3& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleOpaquePipeline) {
        renderSolidSphereInstance(args, batch, glm::vec4(color, 1.0f), pipeline);
    }
    
    void renderWireSphereInstance(RenderArgs* args, gpu::Batch& batch, const glm::vec4& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleWirePipeline);
    void renderWireSphereInstance(RenderArgs* args, gpu::Batch& batch, const glm::vec3& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleWirePipeline) {
        renderWireSphereInstance(args, batch, glm::vec4(color, 1.0f), pipeline);
    }
    
    void renderSolidCubeInstance(RenderArgs* args, gpu::Batch& batch, const glm::vec4& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleOpaquePipeline);
    void renderSolidCubeInstance(RenderArgs* args, gpu::Batch& batch, const glm::vec3& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleOpaquePipeline) {
        renderSolidCubeInstance(args, batch, glm::vec4(color, 1.0f), pipeline);
    }
    
    void renderWireCubeInstance(RenderArgs* args, gpu::Batch& batch, const glm::vec4& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleWirePipeline);
    void renderWireCubeInstance(RenderArgs* args, gpu::Batch& batch, const glm::vec3& color,
                                    const render::ShapePipelinePointer& pipeline = _simpleWirePipeline) {
        renderWireCubeInstance(args, batch, glm::vec4(color, 1.0f), pipeline);
    }
    
    // Dynamic geometry
    void renderShape(gpu::Batch& batch, Shape shape);
    void renderWireShape(gpu::Batch& batch, Shape shape);
    size_t getShapeTriangleCount(Shape shape);

    void renderCube(gpu::Batch& batch);
    void renderWireCube(gpu::Batch& batch);
    size_t getCubeTriangleCount();

    void renderSphere(gpu::Batch& batch);
    void renderWireSphere(gpu::Batch& batch);
    size_t getSphereTriangleCount();

    void renderGrid(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner,
        int majorRows, int majorCols, float majorEdge,
        int minorRows, int minorCols, float minorEdge,
        const glm::vec4& color, bool isLayered, int id);
    void renderGrid(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner,
        int rows, int cols, float edge, const glm::vec4& color, bool isLayered, int id) {
        renderGrid(batch, minCorner, maxCorner, rows, cols, edge, 0, 0, 0.0f, color, isLayered, id);
    }

    void renderBevelCornersRect(gpu::Batch& batch, int x, int y, int width, int height, int bevelDistance, const glm::vec4& color, int id);

    void renderUnitQuad(gpu::Batch& batch, const glm::vec4& color, int id);

    void renderUnitQuad(gpu::Batch& batch, int id) {
        renderUnitQuad(batch, glm::vec4(1), id);
    }

    void renderQuad(gpu::Batch& batch, int x, int y, int width, int height, const glm::vec4& color, int id)
            { renderQuad(batch, glm::vec2(x,y), glm::vec2(x + width, y + height), color, id); }
            
    // TODO: I think there's a bug in this version of the renderQuad() that's not correctly rebuilding the vbos
    // if the color changes by the corners are the same, as evidenced by the audio meter which should turn white
    // when it's clipping
    void renderQuad(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec4& color, int id);

    void renderQuad(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner,
                    const glm::vec2& texCoordMinCorner, const glm::vec2& texCoordMaxCorner, 
                    const glm::vec4& color, int id);

    void renderQuad(gpu::Batch& batch, const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec4& color, int id);

    void renderQuad(gpu::Batch& batch, const glm::vec3& topLeft, const glm::vec3& bottomLeft, 
                    const glm::vec3& bottomRight, const glm::vec3& topRight,
                    const glm::vec2& texCoordTopLeft, const glm::vec2& texCoordBottomLeft,
                    const glm::vec2& texCoordBottomRight, const glm::vec2& texCoordTopRight, 
                    const glm::vec4& color, int id);


    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color, int id)
                    { renderLine(batch, p1, p2, color, color, id); }
    
    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec3& color1, const glm::vec3& color2, int id)
                    { renderLine(batch, p1, p2, glm::vec4(color1, 1.0f), glm::vec4(color2, 1.0f), id); }

    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color, int id)
                    { renderLine(batch, p1, p2, color, color, id); }

    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color1, const glm::vec4& color2, int id);

    void renderGlowLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2,
                    const glm::vec4& color, float glowIntensity, float glowWidth, int id);

    void renderGlowLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color, int id) 
                       { renderGlowLine(batch, p1, p2, color, 1.0f, 0.05f, id); }

    void renderDashedLine(gpu::Batch& batch, const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, int id)
                          { renderDashedLine(batch, start, end, color, 0.05f, 0.025f, id); }

    void renderDashedLine(gpu::Batch& batch, const glm::vec3& start, const glm::vec3& end, const glm::vec4& color,
                          const float dash_length, const float gap_length, int id);

    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2, const glm::vec3& color, int id)
                    { renderLine(batch, p1, p2, glm::vec4(color, 1.0f), id); }

    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color, int id)
                    { renderLine(batch, p1, p2, color, color, id); }

    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2,
                    const glm::vec3& color1, const glm::vec3& color2, int id)
                    { renderLine(batch, p1, p2, glm::vec4(color1, 1.0f), glm::vec4(color2, 1.0f), id); }

    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2,
                                    const glm::vec4& color1, const glm::vec4& color2, int id);

    void updateVertices(int id, const QVector<glm::vec2>& points, const glm::vec4& color);
    void updateVertices(int id, const QVector<glm::vec2>& points, const QVector<glm::vec4>& colors);
    void updateVertices(int id, const QVector<glm::vec3>& points, const glm::vec4& color);
    void updateVertices(int id, const QVector<glm::vec3>& points, const QVector<glm::vec4>& colors);
    void updateVertices(int id, const QVector<glm::vec3>& points, const QVector<glm::vec2>& texCoords, const glm::vec4& color);
    void renderVertices(gpu::Batch& batch, gpu::Primitive primitiveType, int id);

    /// Set a batch to the simple pipeline, returning the previous pipeline
    void useSimpleDrawPipeline(gpu::Batch& batch, bool noBlend = false);

    struct ShapeData {
        size_t _indexOffset{ 0 };
        size_t _indexCount{ 0 };
        size_t _wireIndexOffset{ 0 };
        size_t _wireIndexCount{ 0 };

        gpu::BufferView _positionView;
        gpu::BufferView _normalView;
        gpu::BufferPointer _indices;

        void setupVertices(gpu::BufferPointer& vertexBuffer, const geometry::VertexVector& vertices);
        void setupIndices(gpu::BufferPointer& indexBuffer, const geometry::IndexVector& indices, const geometry::IndexVector& wireIndices);
        void setupBatch(gpu::Batch& batch) const;
        void draw(gpu::Batch& batch) const;
        void drawWire(gpu::Batch& batch) const;
        void drawInstances(gpu::Batch& batch, size_t count) const;
        void drawWireInstances(gpu::Batch& batch, size_t count) const;
    };

    using VShape = std::array<ShapeData, NUM_SHAPES>;

    VShape _shapes;

private:
    GeometryCache();
    virtual ~GeometryCache();
    void buildShapes();

    typedef QPair<int, int> IntPair;
    typedef QPair<unsigned int, unsigned int> VerticesIndices;

    gpu::PipelinePointer _standardDrawPipeline;
    gpu::PipelinePointer _standardDrawPipelineNoBlend;

    gpu::BufferPointer _shapeVertices{ std::make_shared<gpu::Buffer>() };
    gpu::BufferPointer _shapeIndices{ std::make_shared<gpu::Buffer>() };

    class GridSchema {
    public:
        // data is arranged as majorRow, majorCol, minorRow, minorCol
        glm::vec4 period;
        glm::vec4 offset;
        glm::vec4 edge;
    };
    using GridBuffer = gpu::BufferView;
    void useGridPipeline(gpu::Batch& batch, GridBuffer gridBuffer, bool isLayered);
    gpu::PipelinePointer _gridPipeline;
    gpu::PipelinePointer _gridPipelineLayered;
    int _gridSlot;

    class BatchItemDetails {
    public:
        static int population;
        gpu::BufferPointer verticesBuffer;
        gpu::BufferPointer colorBuffer;
        gpu::BufferPointer uniformBuffer;
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

    int _nextID{ 1 };

    QHash<int, Vec3PairVec4Pair> _lastRegisteredQuad3DTexture;
    QHash<int, BatchItemDetails> _registeredQuad3DTextures;

    QHash<int, Vec4PairVec4> _lastRegisteredQuad2DTexture;
    QHash<int, BatchItemDetails> _registeredQuad2DTextures;

    QHash<int, Vec3PairVec4> _lastRegisteredQuad3D;
    QHash<int, BatchItemDetails> _registeredQuad3D;

    QHash<int, Vec4Pair> _lastRegisteredQuad2D;
    QHash<int, BatchItemDetails> _registeredQuad2D;

    QHash<int, Vec3Pair> _lastRegisteredBevelRects;
    QHash<int, BatchItemDetails> _registeredBevelRects;

    QHash<int, Vec3Pair> _lastRegisteredLine3D;
    QHash<int, BatchItemDetails> _registeredLine3DVBOs;

    QHash<int, Vec2Pair> _lastRegisteredLine2D;
    QHash<int, BatchItemDetails> _registeredLine2DVBOs;
    
    QHash<int, BatchItemDetails> _registeredVertices;

    QHash<int, Vec3PairVec2Pair> _lastRegisteredDashedLines;
    QHash<int, BatchItemDetails> _registeredDashedLines;

    QHash<int, Vec2FloatPairPair> _lastRegisteredGridBuffer;
    QHash<int, GridBuffer> _registeredGridBuffers;

    static gpu::ShaderPointer _simpleShader;
    static gpu::ShaderPointer _unlitShader;
    static gpu::ShaderPointer _simpleFadeShader;
    static gpu::ShaderPointer _unlitFadeShader;
    static render::ShapePipelinePointer _simpleOpaquePipeline;
    static render::ShapePipelinePointer _simpleTransparentPipeline;
    static render::ShapePipelinePointer _simpleOpaqueFadePipeline;
    static render::ShapePipelinePointer _simpleTransparentFadePipeline;
    static render::ShapePipelinePointer _simpleOpaqueOverlayPipeline;
    static render::ShapePipelinePointer _simpleTransparentOverlayPipeline;
    static render::ShapePipelinePointer _simpleWirePipeline;
    gpu::PipelinePointer _glowLinePipeline;

    static QHash<SimpleProgramKey, gpu::PipelinePointer> _simplePrograms;

    gpu::ShaderPointer _simpleOpaqueWebBrowserShader;
    gpu::PipelinePointer _simpleOpaqueWebBrowserPipelineNoAA;
    gpu::ShaderPointer _simpleTransparentWebBrowserShader;
    gpu::PipelinePointer _simpleTransparentWebBrowserPipelineNoAA;

    static render::ShapePipelinePointer getShapePipeline(bool textured = false, bool transparent = false, bool culled = true,
        bool unlit = false, bool depthBias = false);
    static render::ShapePipelinePointer getFadingShapePipeline(bool textured = false, bool transparent = false, bool culled = true,
        bool unlit = false, bool depthBias = false);
};

#endif // hifi_GeometryCache_h
