//
//  GeometryCache.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GeometryCache.h"

#include <cmath>

#include <QNetworkReply>
#include <QThreadPool>

#include <FSTReader.h>
#include <NumericalConstants.h>

#include "TextureCache.h"
#include "RenderUtilsLogging.h"

#include "gpu/StandardShaderLib.h"

#include "model/TextureMap.h"

#include "standardTransformPNTC_vert.h"
#include "standardDrawTexture_frag.h"

#include "simple_vert.h"
#include "simple_textured_frag.h"
#include "simple_textured_emisive_frag.h"

#include "grid_frag.h"

//#define WANT_DEBUG

const int GeometryCache::UNKNOWN_ID = -1;


static const int VERTICES_PER_TRIANGLE = 3;

static const gpu::Element POSITION_ELEMENT{ gpu::VEC3, gpu::FLOAT, gpu::XYZ };
static const gpu::Element NORMAL_ELEMENT{ gpu::VEC3, gpu::FLOAT, gpu::XYZ };
static const gpu::Element COLOR_ELEMENT{ gpu::VEC4, gpu::NUINT8, gpu::RGBA };

static gpu::Stream::FormatPointer SOLID_STREAM_FORMAT;
static gpu::Stream::FormatPointer INSTANCED_SOLID_STREAM_FORMAT;

static const uint SHAPE_VERTEX_STRIDE = sizeof(glm::vec3) * 2; // vertices and normals
static const uint SHAPE_NORMALS_OFFSET = sizeof(glm::vec3);
static const gpu::Type SHAPE_INDEX_TYPE = gpu::UINT16;
static const uint SHAPE_INDEX_SIZE = sizeof(gpu::uint16);

void GeometryCache::ShapeData::setupVertices(gpu::BufferPointer& vertexBuffer, const VertexVector& vertices) {
    vertexBuffer->append(vertices);

    _positionView = gpu::BufferView(vertexBuffer, 0,
        vertexBuffer->getSize(), SHAPE_VERTEX_STRIDE, POSITION_ELEMENT);
    _normalView = gpu::BufferView(vertexBuffer, SHAPE_NORMALS_OFFSET,
        vertexBuffer->getSize(), SHAPE_VERTEX_STRIDE, NORMAL_ELEMENT);
}

void GeometryCache::ShapeData::setupIndices(gpu::BufferPointer& indexBuffer, const IndexVector& indices, const IndexVector& wireIndices) {
    _indices = indexBuffer;
    if (!indices.empty()) {
        _indexOffset = indexBuffer->getSize() / SHAPE_INDEX_SIZE;
        _indexCount = indices.size();
        indexBuffer->append(indices);
    }

    if (!wireIndices.empty()) {
        _wireIndexOffset = indexBuffer->getSize() / SHAPE_INDEX_SIZE;
        _wireIndexCount = wireIndices.size();
        indexBuffer->append(wireIndices);
    }
}

void GeometryCache::ShapeData::setupBatch(gpu::Batch& batch) const {
    batch.setInputBuffer(gpu::Stream::POSITION, _positionView);
    batch.setInputBuffer(gpu::Stream::NORMAL, _normalView);
    batch.setIndexBuffer(SHAPE_INDEX_TYPE, _indices, 0);
}

void GeometryCache::ShapeData::draw(gpu::Batch& batch) const {
    if (_indexCount) {
        setupBatch(batch);
        batch.drawIndexed(gpu::TRIANGLES, (gpu::uint32)_indexCount, (gpu::uint32)_indexOffset);
    }
}

void GeometryCache::ShapeData::drawWire(gpu::Batch& batch) const {
    if (_wireIndexCount) {
        setupBatch(batch);
        batch.drawIndexed(gpu::LINES, (gpu::uint32)_wireIndexCount, (gpu::uint32)_wireIndexOffset);
    }
}

void GeometryCache::ShapeData::drawInstances(gpu::Batch& batch, size_t count) const {
    if (_indexCount) {
        setupBatch(batch);
        batch.drawIndexedInstanced((gpu::uint32)count, gpu::TRIANGLES, (gpu::uint32)_indexCount, (gpu::uint32)_indexOffset);
    }
}

void GeometryCache::ShapeData::drawWireInstances(gpu::Batch& batch, size_t count) const {
    if (_wireIndexCount) {
        setupBatch(batch);
        batch.drawIndexedInstanced((gpu::uint32)count, gpu::LINES, (gpu::uint32)_wireIndexCount, (gpu::uint32)_wireIndexOffset);
    }
}

const VertexVector& icosahedronVertices() {
    static const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
    static const float a = 0.5f;
    static const float b = 1.0f / (2.0f * phi);

    static const VertexVector vertices{ //
        vec3(0, b, -a), vec3(-b, a, 0), vec3(b, a, 0), // 
        vec3(0, b, a), vec3(b, a, 0), vec3(-b, a, 0), //
        vec3(0, b, a), vec3(-a, 0, b), vec3(0, -b, a), //
        vec3(0, b, a), vec3(0, -b, a), vec3(a, 0, b),  //
        vec3(0, b, -a), vec3(a, 0, -b), vec3(0, -b, -a),// 
        vec3(0, b, -a), vec3(0, -b, -a), vec3(-a, 0, -b), //
        vec3(0, -b, a), vec3(-b, -a, 0), vec3(b, -a, 0), //
        vec3(0, -b, -a), vec3(b, -a, 0), vec3(-b, -a, 0), //
        vec3(-b, a, 0), vec3(-a, 0, -b),  vec3(-a, 0, b), //
        vec3(-b, -a, 0), vec3(-a, 0, b),  vec3(-a, 0, -b), //
        vec3(b, a, 0), vec3(a, 0, b), vec3(a, 0, -b),   //
        vec3(b, -a, 0), vec3(a, 0, -b), vec3(a, 0, b),  //
        vec3(0, b, a), vec3(-b, a, 0),  vec3(-a, 0, b), //
        vec3(0, b, a), vec3(a, 0, b), vec3(b, a, 0), //
        vec3(0, b, -a), vec3(-a, 0, -b), vec3(-b, a, 0), // 
        vec3(0, b, -a), vec3(b, a, 0),  vec3(a, 0, -b), //
        vec3(0, -b, -a), vec3(-b, -a, 0), vec3(-a, 0, -b), // 
        vec3(0, -b, -a), vec3(a, 0, -b), vec3(b, -a, 0), //
        vec3(0, -b, a), vec3(-a, 0, b),  vec3(-b, -a, 0), //
        vec3(0, -b, a), vec3(b, -a, 0), vec3(a, 0, b)
    }; //
    return vertices;
}

const VertexVector& tetrahedronVertices() {
    static const float a = 1.0f / sqrtf(2.0f);
    static const auto A = vec3(0, 1, a);
    static const auto B = vec3(0, -1, a);
    static const auto C = vec3(1, 0, -a);
    static const auto D = vec3(-1, 0, -a);
    static const VertexVector vertices{
        A, B, C,
        D, B, A,
        C, D, A,
        C, B, D,
    };
    return vertices;
}

static const size_t TESSELTATION_MULTIPLIER = 4;
static const size_t ICOSAHEDRON_TO_SPHERE_TESSELATION_COUNT = 3;
static const size_t VECTOR_TO_VECTOR_WITH_NORMAL_MULTIPLER = 2;


VertexVector tesselate(const VertexVector& startingTriangles, int count) {
    VertexVector triangles = startingTriangles;
    if (0 != (triangles.size() % 3)) {
        throw std::runtime_error("Bad number of vertices for tesselation");
    }

    for (size_t i = 0; i < triangles.size(); ++i) {
        triangles[i] = glm::normalize(triangles[i]);
    }

    VertexVector newTriangles;
    while (count) {
        newTriangles.clear();
        // Tesselation takes one triangle and makes it into 4 triangles
        // See https://en.wikipedia.org/wiki/Space-filling_tree#/media/File:Space_Filling_Tree_Tri_iter_1_2_3.png
        newTriangles.reserve(triangles.size() * TESSELTATION_MULTIPLIER);
        for (size_t i = 0; i < triangles.size(); i += VERTICES_PER_TRIANGLE) {
            const vec3& a = triangles[i];
            const vec3& b = triangles[i + 1];
            const vec3& c = triangles[i + 2];
            vec3 ab = glm::normalize(a + b);
            vec3 bc = glm::normalize(b + c);
            vec3 ca = glm::normalize(c + a);

            newTriangles.push_back(a);
            newTriangles.push_back(ab);
            newTriangles.push_back(ca);

            newTriangles.push_back(b);
            newTriangles.push_back(bc);
            newTriangles.push_back(ab);

            newTriangles.push_back(c);
            newTriangles.push_back(ca);
            newTriangles.push_back(bc);

            newTriangles.push_back(ab);
            newTriangles.push_back(bc);
            newTriangles.push_back(ca);
        }
        triangles.swap(newTriangles);
        --count;
    }
    return triangles;
}

size_t GeometryCache::getShapeTriangleCount(Shape shape) {
    return _shapes[shape]._indexCount / VERTICES_PER_TRIANGLE;
}

size_t GeometryCache::getSphereTriangleCount() {
    return getShapeTriangleCount(Sphere);
}

size_t GeometryCache::getCubeTriangleCount() {
    return getShapeTriangleCount(Cube);
}


// FIXME solids need per-face vertices, but smooth shaded
// components do not.  Find a way to support using draw elements
// or draw arrays as appropriate
// Maybe special case cone and cylinder since they combine flat
// and smooth shading
void GeometryCache::buildShapes() {
    auto vertexBuffer = std::make_shared<gpu::Buffer>();
    auto indexBuffer = std::make_shared<gpu::Buffer>();
    size_t startingIndex = 0;
    
    // Cube 
    startingIndex = _shapeVertices->getSize() / SHAPE_VERTEX_STRIDE;
    {
        ShapeData& shapeData = _shapes[Cube];
        VertexVector vertices;
        // front
        vertices.push_back(vec3(1, 1, 1));
        vertices.push_back(vec3(0, 0, 1));
        vertices.push_back(vec3(-1, 1, 1));
        vertices.push_back(vec3(0, 0, 1));
        vertices.push_back(vec3(-1, -1, 1));
        vertices.push_back(vec3(0, 0, 1));
        vertices.push_back(vec3(1, -1, 1));
        vertices.push_back(vec3(0, 0, 1));

        // right
        vertices.push_back(vec3(1, 1, 1));
        vertices.push_back(vec3(1, 0, 0));
        vertices.push_back(vec3(1, -1, 1));
        vertices.push_back(vec3(1, 0, 0));
        vertices.push_back(vec3(1, -1, -1));
        vertices.push_back(vec3(1, 0, 0));
        vertices.push_back(vec3(1, 1, -1));
        vertices.push_back(vec3(1, 0, 0));

        // top
        vertices.push_back(vec3(1, 1, 1));
        vertices.push_back(vec3(0, 1, 0));
        vertices.push_back(vec3(1, 1, -1));
        vertices.push_back(vec3(0, 1, 0));
        vertices.push_back(vec3(-1, 1, -1));
        vertices.push_back(vec3(0, 1, 0));
        vertices.push_back(vec3(-1, 1, 1));
        vertices.push_back(vec3(0, 1, 0));

        // left
        vertices.push_back(vec3(-1, 1, 1));
        vertices.push_back(vec3(-1, 0, 0));
        vertices.push_back(vec3(-1, 1, -1));
        vertices.push_back(vec3(-1, 0, 0));
        vertices.push_back(vec3(-1, -1, -1));
        vertices.push_back(vec3(-1, 0, 0));
        vertices.push_back(vec3(-1, -1, 1));
        vertices.push_back(vec3(-1, 0, 0));

        // bottom
        vertices.push_back(vec3(-1, -1, -1));
        vertices.push_back(vec3(0, -1, 0));
        vertices.push_back(vec3(1, -1, -1));
        vertices.push_back(vec3(0, -1, 0));
        vertices.push_back(vec3(1, -1, 1));
        vertices.push_back(vec3(0, -1, 0));
        vertices.push_back(vec3(-1, -1, 1));
        vertices.push_back(vec3(0, -1, 0));

        // back
        vertices.push_back(vec3(1, -1, -1));
        vertices.push_back(vec3(0, 0, -1));
        vertices.push_back(vec3(-1, -1, -1));
        vertices.push_back(vec3(0, 0, -1));
        vertices.push_back(vec3(-1, 1, -1));
        vertices.push_back(vec3(0, 0, -1));
        vertices.push_back(vec3(1, 1, -1));
        vertices.push_back(vec3(0, 0, -1));

        static const size_t VERTEX_FORMAT_SIZE = 2;
        static const size_t VERTEX_OFFSET = 0;

        for (size_t i = 0; i < vertices.size(); ++i) {
            auto vertexIndex = i;
            // Make a unit cube by having the vertices (at index N) 
            // while leaving the normals (at index N + 1) alone
            if (VERTEX_OFFSET == vertexIndex % VERTEX_FORMAT_SIZE) {
                vertices[vertexIndex] *= 0.5f;
            }
        }
        shapeData.setupVertices(_shapeVertices, vertices);

        IndexVector indices{
            0, 1, 2, 2, 3, 0, // front
            4, 5, 6, 6, 7, 4, // right
            8, 9, 10, 10, 11, 8, // top
            12, 13, 14, 14, 15, 12, // left
            16, 17, 18, 18, 19, 16, // bottom
            20, 21, 22, 22, 23, 20  // back
        };
        for (auto& index : indices) {
            index += (uint16_t)startingIndex;
        }

        IndexVector wireIndices{
            0, 1, 1, 2, 2, 3, 3, 0, // front
            20, 21, 21, 22, 22, 23, 23, 20, // back
            0, 23, 1, 22, 2, 21, 3, 20 // sides
        };

        for (size_t i = 0; i < wireIndices.size(); ++i) {
            indices[i] += (uint16_t)startingIndex;
        }

        shapeData.setupIndices(_shapeIndices, indices, wireIndices);
    }

    // Tetrahedron
    startingIndex = _shapeVertices->getSize() / SHAPE_VERTEX_STRIDE;
    {
        ShapeData& shapeData = _shapes[Tetrahedron];
        size_t vertexCount = 4;
        VertexVector vertices;
        {
            VertexVector originalVertices = tetrahedronVertices();
            vertexCount = originalVertices.size();
            vertices.reserve(originalVertices.size() * VECTOR_TO_VECTOR_WITH_NORMAL_MULTIPLER);
            for (size_t i = 0; i < originalVertices.size(); i += VERTICES_PER_TRIANGLE) {
                auto triangleStartIndex = i;
                vec3 faceNormal;
                for (size_t j = 0; j < VERTICES_PER_TRIANGLE; ++j) {
                    auto triangleVertexIndex = j;
                    auto vertexIndex = triangleStartIndex + triangleVertexIndex;
                    faceNormal += originalVertices[vertexIndex];
                }
                faceNormal = glm::normalize(faceNormal);
                for (size_t j = 0; j < VERTICES_PER_TRIANGLE; ++j) {
                    auto triangleVertexIndex = j;
                    auto vertexIndex = triangleStartIndex + triangleVertexIndex;
                    vertices.push_back(glm::normalize(originalVertices[vertexIndex]));
                    vertices.push_back(faceNormal);
                }
            }
        }
        shapeData.setupVertices(_shapeVertices, vertices);

        IndexVector indices;
        for (size_t i = 0; i < vertexCount; i += VERTICES_PER_TRIANGLE) {
            auto triangleStartIndex = i;
            for (size_t j = 0; j < VERTICES_PER_TRIANGLE; ++j) {
                auto triangleVertexIndex = j;
                auto vertexIndex = triangleStartIndex + triangleVertexIndex;
                indices.push_back((uint16_t)(vertexIndex + startingIndex));
            }
        }

        IndexVector wireIndices{
            0, 1, 1, 2, 2, 0,
            0, 3, 1, 3, 2, 3,
        };

        for (size_t i = 0; i < wireIndices.size(); ++i) {
            wireIndices[i] += (uint16_t)startingIndex;
        }

        shapeData.setupIndices(_shapeIndices, indices, wireIndices);
    }

    // Sphere
    // FIXME this uses way more vertices than required.  Should find a way to calculate the indices
    // using shared vertices for better vertex caching
    startingIndex = _shapeVertices->getSize() / SHAPE_VERTEX_STRIDE;
    {
        ShapeData& shapeData = _shapes[Sphere];
        VertexVector vertices;
        IndexVector indices;
        {
            VertexVector originalVertices = tesselate(icosahedronVertices(), ICOSAHEDRON_TO_SPHERE_TESSELATION_COUNT);
            vertices.reserve(originalVertices.size() * VECTOR_TO_VECTOR_WITH_NORMAL_MULTIPLER);
            for (size_t i = 0; i < originalVertices.size(); i += VERTICES_PER_TRIANGLE) {
                auto triangleStartIndex = i;
                for (int j = 0; j < VERTICES_PER_TRIANGLE; ++j) {
                    auto triangleVertexIndex = j;
                    auto vertexIndex = triangleStartIndex + triangleVertexIndex;
                    const auto& vertex = originalVertices[i + j];
                    // Spheres use the same values for vertices and normals
                    vertices.push_back(vertex);
                    vertices.push_back(vertex);
                    indices.push_back((uint16_t)(vertexIndex + startingIndex));
                }
            }
        }
        
        shapeData.setupVertices(_shapeVertices, vertices);
        // FIXME don't use solid indices for wire drawing.  
        shapeData.setupIndices(_shapeIndices, indices, indices);
    }

    // Icosahedron
    startingIndex = _shapeVertices->getSize() / SHAPE_VERTEX_STRIDE;
    {
        ShapeData& shapeData = _shapes[Icosahedron];

        VertexVector vertices;
        IndexVector indices;
        {
            const VertexVector& originalVertices = icosahedronVertices();
            vertices.reserve(originalVertices.size() * VECTOR_TO_VECTOR_WITH_NORMAL_MULTIPLER);
            for (size_t i = 0; i < originalVertices.size(); i += 3) {
                auto triangleStartIndex = i;
                vec3 faceNormal;
                for (int j = 0; j < VERTICES_PER_TRIANGLE; ++j) {
                    auto triangleVertexIndex = j;
                    auto vertexIndex = triangleStartIndex + triangleVertexIndex;
                    faceNormal += originalVertices[vertexIndex];
                }
                faceNormal = glm::normalize(faceNormal);
                for (int j = 0; j < VERTICES_PER_TRIANGLE; ++j) {
                    auto triangleVertexIndex = j;
                    auto vertexIndex = triangleStartIndex + triangleVertexIndex;
                    vertices.push_back(glm::normalize(originalVertices[vertexIndex]));
                    vertices.push_back(faceNormal);
                    indices.push_back((uint16_t)(vertexIndex + startingIndex));
                }
            }
        }

        shapeData.setupVertices(_shapeVertices, vertices);
        // FIXME don't use solid indices for wire drawing.  
        shapeData.setupIndices(_shapeIndices, indices, indices);
    }

    // Line
    startingIndex = _shapeVertices->getSize() / SHAPE_VERTEX_STRIDE;
    {
        ShapeData& shapeData = _shapes[Line];
        shapeData.setupVertices(_shapeVertices, VertexVector{
            vec3(-0.5, 0, 0), vec3(-0.5f, 0, 0),
            vec3(0.5f, 0, 0), vec3(0.5f, 0, 0)
        });
        IndexVector wireIndices;
        // Only two indices
        wireIndices.push_back(0 + (uint16_t)startingIndex);
        wireIndices.push_back(1 + (uint16_t)startingIndex);

        shapeData.setupIndices(_shapeIndices, IndexVector(), wireIndices);
    }

    // Not implememented yet:

    //Triangle,
    //Quad,
    //Circle,
    //Octahetron,
    //Dodecahedron,
    //Torus,
    //Cone,
    //Cylinder,
}

gpu::Stream::FormatPointer& getSolidStreamFormat() {
    if (!SOLID_STREAM_FORMAT) {
        SOLID_STREAM_FORMAT = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
        SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, POSITION_ELEMENT);
        SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::NORMAL, gpu::Stream::NORMAL, NORMAL_ELEMENT);
    }
    return SOLID_STREAM_FORMAT;
}

gpu::Stream::FormatPointer& getInstancedSolidStreamFormat() {
    if (!INSTANCED_SOLID_STREAM_FORMAT) {
        INSTANCED_SOLID_STREAM_FORMAT = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
        INSTANCED_SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, POSITION_ELEMENT);
        INSTANCED_SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::NORMAL, gpu::Stream::NORMAL, NORMAL_ELEMENT);
        INSTANCED_SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::COLOR, gpu::Stream::COLOR, COLOR_ELEMENT, 0, gpu::Stream::PER_INSTANCE);
    }
    return INSTANCED_SOLID_STREAM_FORMAT;
}

render::ShapePipelinePointer GeometryCache::_simplePipeline;

GeometryCache::GeometryCache() :
    _nextID(0)
{
    buildShapes();
    GeometryCache::_simplePipeline =
        std::make_shared<render::ShapePipeline>(getSimplePipeline(), nullptr,
            [](const render::ShapePipeline&, gpu::Batch& batch) {
                // Set the defaults needed for a simple program
                batch.setResourceTexture(render::ShapePipeline::Slot::ALBEDO_MAP,
                    DependencyManager::get<TextureCache>()->getWhiteTexture());
                batch.setResourceTexture(render::ShapePipeline::Slot::NORMAL_FITTING_MAP,
                    DependencyManager::get<TextureCache>()->getNormalFittingTexture());
            }
        );
}

GeometryCache::~GeometryCache() {
    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "GeometryCache::~GeometryCache()... ";
        qCDebug(renderutils) << "    _registeredLine3DVBOs.size():" << _registeredLine3DVBOs.size();
        qCDebug(renderutils) << "    _line3DVBOs.size():" << _line3DVBOs.size();
        qCDebug(renderutils) << "    BatchItemDetails... population:" << GeometryCache::BatchItemDetails::population;
    #endif //def WANT_DEBUG
}

void setupBatchInstance(gpu::Batch& batch, gpu::BufferPointer colorBuffer) {
    gpu::BufferView colorView(colorBuffer, COLOR_ELEMENT);
    batch.setInputBuffer(gpu::Stream::COLOR, colorView);
}

void GeometryCache::renderShape(gpu::Batch& batch, Shape shape) {
    batch.setInputFormat(getSolidStreamFormat());
    _shapes[shape].draw(batch);
}

void GeometryCache::renderWireShape(gpu::Batch& batch, Shape shape) {
    batch.setInputFormat(getSolidStreamFormat());
    _shapes[shape].drawWire(batch);
}

void GeometryCache::renderShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& colorBuffer) {
    batch.setInputFormat(getInstancedSolidStreamFormat());
    setupBatchInstance(batch, colorBuffer);
    _shapes[shape].drawInstances(batch, count);
}

void GeometryCache::renderWireShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& colorBuffer) {
    batch.setInputFormat(getInstancedSolidStreamFormat());
    setupBatchInstance(batch, colorBuffer);
    _shapes[shape].drawWireInstances(batch, count);
}

void GeometryCache::renderCube(gpu::Batch& batch) {
    renderShape(batch, Cube);
}

void GeometryCache::renderWireCube(gpu::Batch& batch) {
    renderWireShape(batch, Cube);
}

void GeometryCache::renderSphere(gpu::Batch& batch) {
    renderShape(batch, Sphere);
}

void GeometryCache::renderWireSphere(gpu::Batch& batch) {
    renderWireShape(batch, Sphere);
}

void GeometryCache::renderGrid(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner,
                            int majorRows, int majorCols, float majorEdge,
                            int minorRows, int minorCols, float minorEdge,
                            const glm::vec4& color, bool isLayered, int id) {
    static const glm::vec2 MIN_TEX_COORD(0.0f, 0.0f);
    static const glm::vec2 MAX_TEX_COORD(1.0f, 1.0f);

    bool registered = (id != UNKNOWN_ID);
    Vec2FloatPair majorKey(glm::vec2(majorRows, majorCols), majorEdge);
    Vec2FloatPair minorKey(glm::vec2(minorRows, minorCols), minorEdge);
    Vec2FloatPairPair key(majorKey, minorKey);

    // Make the gridbuffer
    if ((registered && (!_registeredGridBuffers.contains(id) || _lastRegisteredGridBuffer[id] != key)) ||
        (!registered && !_gridBuffers.contains(key))) {
        GridSchema gridSchema;
        GridBuffer gridBuffer = std::make_shared<gpu::Buffer>(sizeof(GridSchema), (const gpu::Byte*) &gridSchema);

        if (registered && _registeredGridBuffers.contains(id)) {
            gridBuffer = _registeredGridBuffers[id];
        }

        if (registered) {
            _registeredGridBuffers[id] = gridBuffer;
            _lastRegisteredGridBuffer[id] = key;
        } else {
            _gridBuffers[key] = gridBuffer;
        }

        gridBuffer.edit<GridSchema>().period = glm::vec4(majorRows, majorCols, minorRows, minorCols);
        gridBuffer.edit<GridSchema>().offset.x = -(majorEdge / majorRows) / 2;
        gridBuffer.edit<GridSchema>().offset.y = -(majorEdge / majorCols) / 2;
        gridBuffer.edit<GridSchema>().offset.z = -(minorEdge / minorRows) / 2;
        gridBuffer.edit<GridSchema>().offset.w = -(minorEdge / minorCols) / 2;
        gridBuffer.edit<GridSchema>().edge = glm::vec4(glm::vec2(majorEdge),
            // If rows or columns are not set, do not draw minor gridlines
            glm::vec2((minorRows != 0 && minorCols != 0) ? minorEdge : 0.0f));
    }

    // Set the grid pipeline
    useGridPipeline(batch, registered ? _registeredGridBuffers[id] : _gridBuffers[key], isLayered);

    renderQuad(batch, minCorner, maxCorner, MIN_TEX_COORD, MAX_TEX_COORD, color, id);
}

void GeometryCache::updateVertices(int id, const QVector<glm::vec2>& points, const glm::vec4& color) {
    BatchItemDetails& details = _registeredVertices[id];

    if (details.isCreated) {
        details.clear();
        #ifdef WANT_DEBUG
            qCDebug(renderutils) << "updateVertices()... RELEASING REGISTERED";
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 2 + 3; // vertices + normals
    const int NUM_POS_COORDS = 2;
    const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);
    details.isCreated = true;
    details.vertices = points.size();
    details.vertexSize = FLOATS_PER_VERTEX;
    
    auto verticesBuffer = std::make_shared<gpu::Buffer>();
    auto colorBuffer = std::make_shared<gpu::Buffer>();
    auto streamFormat = std::make_shared<gpu::Stream::Format>();
    auto stream = std::make_shared<gpu::BufferStream>();

    details.verticesBuffer = verticesBuffer;
    details.colorBuffer = colorBuffer;
    details.streamFormat = streamFormat;
    details.stream = stream;

    details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
    details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
    details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

    details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
    details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);

    details.vertices = points.size();
    details.vertexSize = FLOATS_PER_VERTEX;

    int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                        ((int(color.y * 255.0f) & 0xFF) << 8) |
                        ((int(color.z * 255.0f) & 0xFF) << 16) |
                        ((int(color.w * 255.0f) & 0xFF) << 24);

    float* vertexData = new float[details.vertices * FLOATS_PER_VERTEX];
    float* vertex = vertexData;

    int* colorData = new int[details.vertices];
    int* colorDataAt = colorData;

    const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
    foreach (const glm::vec2& point, points) {
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        *(vertex++) = NORMAL.x;
        *(vertex++) = NORMAL.y;
        *(vertex++) = NORMAL.z;
        
        *(colorDataAt++) = compactColor;
    }

    details.verticesBuffer->append(sizeof(float) * FLOATS_PER_VERTEX * details.vertices, (gpu::Byte*) vertexData);
    details.colorBuffer->append(sizeof(int) * details.vertices, (gpu::Byte*) colorData);
    delete[] vertexData;
    delete[] colorData;

    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "new registered linestrip buffer made -- _registeredVertices.size():" << _registeredVertices.size();
    #endif
}

void GeometryCache::updateVertices(int id, const QVector<glm::vec3>& points, const glm::vec4& color) {
    BatchItemDetails& details = _registeredVertices[id];
    if (details.isCreated) {
        details.clear();
        #ifdef WANT_DEBUG
            qCDebug(renderutils) << "updateVertices()... RELEASING REGISTERED";
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 3 + 3; // vertices + normals
    const int NUM_POS_COORDS = 3;
    const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);
    details.isCreated = true;
    details.vertices = points.size();
    details.vertexSize = FLOATS_PER_VERTEX;
    
    auto verticesBuffer = std::make_shared<gpu::Buffer>();
    auto colorBuffer = std::make_shared<gpu::Buffer>();
    auto streamFormat = std::make_shared<gpu::Stream::Format>();
    auto stream = std::make_shared<gpu::BufferStream>();

    details.verticesBuffer = verticesBuffer;
    details.colorBuffer = colorBuffer;
    details.streamFormat = streamFormat;
    details.stream = stream;

    details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
    details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
    details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

    details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
    details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);

    details.vertices = points.size();
    details.vertexSize = FLOATS_PER_VERTEX;

    int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                        ((int(color.y * 255.0f) & 0xFF) << 8) |
                        ((int(color.z * 255.0f) & 0xFF) << 16) |
                        ((int(color.w * 255.0f) & 0xFF) << 24);

    float* vertexData = new float[details.vertices * FLOATS_PER_VERTEX];
    float* vertex = vertexData;

    int* colorData = new int[details.vertices];
    int* colorDataAt = colorData;

    const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
    foreach (const glm::vec3& point, points) {
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        *(vertex++) = point.z;
        *(vertex++) = NORMAL.x;
        *(vertex++) = NORMAL.y;
        *(vertex++) = NORMAL.z;
        
        *(colorDataAt++) = compactColor;
    }

    details.verticesBuffer->append(sizeof(float) * FLOATS_PER_VERTEX * details.vertices, (gpu::Byte*) vertexData);
    details.colorBuffer->append(sizeof(int) * details.vertices, (gpu::Byte*) colorData);
    delete[] vertexData;
    delete[] colorData;

    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "new registered linestrip buffer made -- _registeredVertices.size():" << _registeredVertices.size();
    #endif
}

void GeometryCache::updateVertices(int id, const QVector<glm::vec3>& points, const QVector<glm::vec2>& texCoords, const glm::vec4& color) {
    BatchItemDetails& details = _registeredVertices[id];

    if (details.isCreated) {
        details.clear();
        #ifdef WANT_DEBUG
            qCDebug(renderutils) << "updateVertices()... RELEASING REGISTERED";
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 3 + 3 + 2; // vertices + normals + tex coords
    const int NUM_POS_COORDS = 3;
    const int NUM_NORMAL_COORDS = 3;
    const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);
    const int VERTEX_TEX_OFFSET = VERTEX_NORMAL_OFFSET + NUM_NORMAL_COORDS * sizeof(float);
    details.isCreated = true;
    details.vertices = points.size();
    details.vertexSize = FLOATS_PER_VERTEX;
    
    auto verticesBuffer = std::make_shared<gpu::Buffer>();
    auto colorBuffer = std::make_shared<gpu::Buffer>();
    auto streamFormat = std::make_shared<gpu::Stream::Format>();
    auto stream = std::make_shared<gpu::BufferStream>();

    details.verticesBuffer = verticesBuffer;
    details.colorBuffer = colorBuffer;
    details.streamFormat = streamFormat;
    details.stream = stream;

    details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
    details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
    details.streamFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), VERTEX_TEX_OFFSET);
    details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

    details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
    details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);

    assert(points.size() == texCoords.size());

    details.vertices = points.size();
    details.vertexSize = FLOATS_PER_VERTEX;

    int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                        ((int(color.y * 255.0f) & 0xFF) << 8) |
                        ((int(color.z * 255.0f) & 0xFF) << 16) |
                        ((int(color.w * 255.0f) & 0xFF) << 24);

    float* vertexData = new float[details.vertices * FLOATS_PER_VERTEX];
    float* vertex = vertexData;

    int* colorData = new int[details.vertices];
    int* colorDataAt = colorData;

    const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
    for (int i = 0; i < points.size(); i++) {
        glm::vec3 point = points[i];
        glm::vec2 texCoord = texCoords[i];
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        *(vertex++) = point.z;
        *(vertex++) = NORMAL.x;
        *(vertex++) = NORMAL.y;
        *(vertex++) = NORMAL.z;
        *(vertex++) = texCoord.x;
        *(vertex++) = texCoord.y;

        *(colorDataAt++) = compactColor;
    }

    details.verticesBuffer->append(sizeof(float) * FLOATS_PER_VERTEX * details.vertices, (gpu::Byte*) vertexData);
    details.colorBuffer->append(sizeof(int) * details.vertices, (gpu::Byte*) colorData);
    delete[] vertexData;
    delete[] colorData;

    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "new registered linestrip buffer made -- _registeredVertices.size():" << _registeredVertices.size();
    #endif
}

void GeometryCache::renderVertices(gpu::Batch& batch, gpu::Primitive primitiveType, int id) {
    BatchItemDetails& details = _registeredVertices[id];
    if (details.isCreated) {
        batch.setInputFormat(details.streamFormat);
        batch.setInputStream(0, *details.stream);
        batch.draw(primitiveType, details.vertices, 0);
    }
}


void GeometryCache::renderBevelCornersRect(gpu::Batch& batch, int x, int y, int width, int height, int bevelDistance, const glm::vec4& color, int id) {
    bool registered = (id != UNKNOWN_ID);
    Vec3Pair key(glm::vec3(x, y, 0.0f), glm::vec3(width, height, bevelDistance));
    BatchItemDetails& details = registered ? _registeredBevelRects[id] : _bevelRects[key];
    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        Vec3Pair& lastKey = _lastRegisteredBevelRects[id];
        if (lastKey != key) {
            details.clear();
            _lastRegisteredBevelRects[id] = key;  
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderBevelCornersRect()... RELEASING REGISTERED";
            #endif // def WANT_DEBUG
        }
        #ifdef WANT_DEBUG
        else {
            qCDebug(renderutils) << "renderBevelCornersRect()... REUSING PREVIOUSLY REGISTERED";
        }
        #endif // def WANT_DEBUG
    }

    if (!details.isCreated) {
        static const int FLOATS_PER_VERTEX = 2; // vertices
        static const int NUM_VERTICES = 8;
        static const int NUM_FLOATS = NUM_VERTICES * FLOATS_PER_VERTEX;
        
        details.isCreated = true;
        details.vertices = NUM_VERTICES;
        details.vertexSize = FLOATS_PER_VERTEX;
        
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        auto streamFormat = std::make_shared<gpu::Stream::Format>();
        auto stream = std::make_shared<gpu::BufferStream>();

        details.verticesBuffer = verticesBuffer;
        details.colorBuffer = colorBuffer;
        details.streamFormat = streamFormat;
        details.stream = stream;
    
        details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ));
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        float vertexBuffer[NUM_FLOATS]; // only vertices, no normals because we're a 2D quad
        int vertexPoint = 0;

        // Triangle strip points
        //      3 ------ 5      //
        //    /            \    //
        //  1                7  //
        //  |                |  //
        //  2                8  //
        //    \            /    //
        //      4 ------ 6      //
        
        // 1
        vertexBuffer[vertexPoint++] = x;
        vertexBuffer[vertexPoint++] = y + height - bevelDistance;
        // 2
        vertexBuffer[vertexPoint++] = x;
        vertexBuffer[vertexPoint++] = y + bevelDistance;
        // 3
        vertexBuffer[vertexPoint++] = x + bevelDistance;
        vertexBuffer[vertexPoint++] = y + height;
        // 4
        vertexBuffer[vertexPoint++] = x + bevelDistance;
        vertexBuffer[vertexPoint++] = y;
        // 5
        vertexBuffer[vertexPoint++] = x + width - bevelDistance;
        vertexBuffer[vertexPoint++] = y + height;
        // 6
        vertexBuffer[vertexPoint++] = x + width - bevelDistance;
        vertexBuffer[vertexPoint++] = y;
        // 7
        vertexBuffer[vertexPoint++] = x + width;
        vertexBuffer[vertexPoint++] = y + height - bevelDistance;
        // 8
        vertexBuffer[vertexPoint++] = x + width;
        vertexBuffer[vertexPoint++] = y + bevelDistance;
        
        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);
        int colors[NUM_VERTICES] = { compactColor, compactColor, compactColor, compactColor,
                                     compactColor, compactColor, compactColor, compactColor };


        details.verticesBuffer->append(sizeof(vertexBuffer), (gpu::Byte*) vertexBuffer);
        details.colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);
    }

    batch.setInputFormat(details.streamFormat);
    batch.setInputStream(0, *details.stream);
    batch.draw(gpu::TRIANGLE_STRIP, details.vertices, 0);
}

void GeometryCache::renderQuad(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec4& color, int id) {
    bool registered = (id != UNKNOWN_ID);
    Vec4Pair key(glm::vec4(minCorner.x, minCorner.y, maxCorner.x, maxCorner.y), color);
    BatchItemDetails& details = registered ? _registeredQuad2D[id] : _quad2D[key];

    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        Vec4Pair & lastKey = _lastRegisteredQuad2D[id];
        if (lastKey != key) {
            details.clear();
            _lastRegisteredQuad2D[id] = key;  
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderQuad() 2D ... RELEASING REGISTERED";
            #endif // def WANT_DEBUG
        }
        #ifdef WANT_DEBUG
        else {
            qCDebug(renderutils) << "renderQuad() 2D ... REUSING PREVIOUSLY REGISTERED";
        }
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 2 + 3; // vertices + normals
    const int VERTICES = 4; // 1 quad = 4 vertices
    const int NUM_POS_COORDS = 2;
    const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);

    if (!details.isCreated) {

        details.isCreated = true;
        details.vertices = VERTICES;
        details.vertexSize = FLOATS_PER_VERTEX;
        
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        auto streamFormat = std::make_shared<gpu::Stream::Format>();
        auto stream = std::make_shared<gpu::BufferStream>();

        details.verticesBuffer = verticesBuffer;
        details.colorBuffer = colorBuffer;
        details.streamFormat = streamFormat;
        details.stream = stream;
    
        details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
        details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
        float vertexBuffer[VERTICES * FLOATS_PER_VERTEX] = {    
            minCorner.x, minCorner.y, NORMAL.x, NORMAL.y, NORMAL.z,
            maxCorner.x, minCorner.y, NORMAL.x, NORMAL.y, NORMAL.z,
            minCorner.x, maxCorner.y, NORMAL.x, NORMAL.y, NORMAL.z,
            maxCorner.x, maxCorner.y, NORMAL.x, NORMAL.y, NORMAL.z,
        };

        const int NUM_COLOR_SCALARS_PER_QUAD = 4;
        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);
        int colors[NUM_COLOR_SCALARS_PER_QUAD] = { compactColor, compactColor, compactColor, compactColor };

        details.verticesBuffer->append(sizeof(vertexBuffer), (gpu::Byte*) vertexBuffer);
        details.colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);
    }

    batch.setInputFormat(details.streamFormat);
    batch.setInputStream(0, *details.stream);
    batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
}

void GeometryCache::renderUnitQuad(gpu::Batch& batch, const glm::vec4& color, int id) {
    static const glm::vec2 topLeft(-1, 1);
    static const glm::vec2 bottomRight(1, -1);
    static const glm::vec2 texCoordTopLeft(0.0f, 1.0f);
    static const glm::vec2 texCoordBottomRight(1.0f, 0.0f);
    renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color, id);
}


void GeometryCache::renderQuad(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner,
                    const glm::vec2& texCoordMinCorner, const glm::vec2& texCoordMaxCorner, 
                    const glm::vec4& color, int id) {

    bool registered = (id != UNKNOWN_ID);
    Vec4PairVec4 key(Vec4Pair(glm::vec4(minCorner.x, minCorner.y, maxCorner.x, maxCorner.y),
                              glm::vec4(texCoordMinCorner.x, texCoordMinCorner.y, texCoordMaxCorner.x, texCoordMaxCorner.y)), 
                              color);
    BatchItemDetails& details = registered ? _registeredQuad2DTextures[id] : _quad2DTextures[key];

    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        Vec4PairVec4& lastKey = _lastRegisteredQuad2DTexture[id];
        if (lastKey != key) {
            details.clear();
            _lastRegisteredQuad2DTexture[id] = key;  
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderQuad() 2D+texture ... RELEASING REGISTERED";
            #endif // def WANT_DEBUG
        }
        #ifdef WANT_DEBUG
        else {
            qCDebug(renderutils) << "renderQuad() 2D+texture ... REUSING PREVIOUSLY REGISTERED";
        }
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 2 + 3 + 2; // vertices + normals + tex coords
    const int VERTICES = 4; // 1 quad = 4 vertices
    const int NUM_POS_COORDS = 2;
    const int NUM_NORMAL_COORDS = 3;
    const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);
    const int VERTEX_TEXCOORD_OFFSET = VERTEX_NORMAL_OFFSET + NUM_NORMAL_COORDS * sizeof(float);

    if (!details.isCreated) {

        details.isCreated = true;
        details.vertices = VERTICES;
        details.vertexSize = FLOATS_PER_VERTEX;
        
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        auto colorBuffer = std::make_shared<gpu::Buffer>();

        auto streamFormat = std::make_shared<gpu::Stream::Format>();
        auto stream = std::make_shared<gpu::BufferStream>();

        details.verticesBuffer = verticesBuffer;
        details.colorBuffer = colorBuffer;

        details.streamFormat = streamFormat;
        details.stream = stream;
    
        // zzmp: fix the normal across all renderQuad
        details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
        details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), VERTEX_TEXCOORD_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
        float vertexBuffer[VERTICES * FLOATS_PER_VERTEX] = {    
            minCorner.x, minCorner.y, NORMAL.x, NORMAL.y, NORMAL.z, texCoordMinCorner.x, texCoordMinCorner.y,
            maxCorner.x, minCorner.y, NORMAL.x, NORMAL.y, NORMAL.z, texCoordMaxCorner.x, texCoordMinCorner.y,
            minCorner.x, maxCorner.y, NORMAL.x, NORMAL.y, NORMAL.z, texCoordMinCorner.x, texCoordMaxCorner.y,
            maxCorner.x, maxCorner.y, NORMAL.x, NORMAL.y, NORMAL.z, texCoordMaxCorner.x, texCoordMaxCorner.y,
        };


        const int NUM_COLOR_SCALARS_PER_QUAD = 4;
        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);
        int colors[NUM_COLOR_SCALARS_PER_QUAD] = { compactColor, compactColor, compactColor, compactColor };

        details.verticesBuffer->append(sizeof(vertexBuffer), (gpu::Byte*) vertexBuffer);
        details.colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);
    }

    batch.setInputFormat(details.streamFormat);
    batch.setInputStream(0, *details.stream);
    batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
}

void GeometryCache::renderQuad(gpu::Batch& batch, const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec4& color, int id) {
    bool registered = (id != UNKNOWN_ID);
    Vec3PairVec4 key(Vec3Pair(minCorner, maxCorner), color);
    BatchItemDetails& details = registered ? _registeredQuad3D[id] : _quad3D[key];

    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        Vec3PairVec4& lastKey = _lastRegisteredQuad3D[id];
        if (lastKey != key) {
            details.clear();
            _lastRegisteredQuad3D[id] = key;  
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderQuad() 3D ... RELEASING REGISTERED";
            #endif // def WANT_DEBUG
        }
        #ifdef WANT_DEBUG
        else {
            qCDebug(renderutils) << "renderQuad() 3D ... REUSING PREVIOUSLY REGISTERED";
        }
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 3 + 3; // vertices + normals
    const int VERTICES = 4; // 1 quad = 4 vertices
    const int NUM_POS_COORDS = 3;
    const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);

    if (!details.isCreated) {

        details.isCreated = true;
        details.vertices = VERTICES;
        details.vertexSize = FLOATS_PER_VERTEX;
        
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        auto colorBuffer = std::make_shared<gpu::Buffer>();

        auto streamFormat = std::make_shared<gpu::Stream::Format>();
        auto stream = std::make_shared<gpu::BufferStream>();

        details.verticesBuffer = verticesBuffer;
        details.colorBuffer = colorBuffer;

        details.streamFormat = streamFormat;
        details.stream = stream;
    
        details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
        float vertexBuffer[VERTICES * FLOATS_PER_VERTEX] = {    
            minCorner.x, minCorner.y, minCorner.z, NORMAL.x, NORMAL.y, NORMAL.z,
            maxCorner.x, minCorner.y, minCorner.z, NORMAL.x, NORMAL.y, NORMAL.z,
            minCorner.x, maxCorner.y, maxCorner.z, NORMAL.x, NORMAL.y, NORMAL.z,
            maxCorner.x, maxCorner.y, maxCorner.z, NORMAL.x, NORMAL.y, NORMAL.z,
        };

        const int NUM_COLOR_SCALARS_PER_QUAD = 4;
        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);
        int colors[NUM_COLOR_SCALARS_PER_QUAD] = { compactColor, compactColor, compactColor, compactColor };

        details.verticesBuffer->append(sizeof(vertexBuffer), (gpu::Byte*) vertexBuffer);
        details.colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);
    }

    batch.setInputFormat(details.streamFormat);
    batch.setInputStream(0, *details.stream);
    batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
}

void GeometryCache::renderQuad(gpu::Batch& batch, const glm::vec3& topLeft, const glm::vec3& bottomLeft, 
                    const glm::vec3& bottomRight, const glm::vec3& topRight,
                    const glm::vec2& texCoordTopLeft, const glm::vec2& texCoordBottomLeft,
                    const glm::vec2& texCoordBottomRight, const glm::vec2& texCoordTopRight, 
                    const glm::vec4& color, int id) {

    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "renderQuad() vec3 + texture VBO...";
        qCDebug(renderutils) << "    topLeft:" << topLeft;
        qCDebug(renderutils) << "    bottomLeft:" << bottomLeft;
        qCDebug(renderutils) << "    bottomRight:" << bottomRight;
        qCDebug(renderutils) << "    topRight:" << topRight;
        qCDebug(renderutils) << "    texCoordTopLeft:" << texCoordTopLeft;
        qCDebug(renderutils) << "    texCoordBottomRight:" << texCoordBottomRight;
        qCDebug(renderutils) << "    color:" << color;
    #endif //def WANT_DEBUG
    
    bool registered = (id != UNKNOWN_ID);
    Vec3PairVec4Pair key(Vec3Pair(topLeft, bottomRight),
                            Vec4Pair(glm::vec4(texCoordTopLeft.x,texCoordTopLeft.y,texCoordBottomRight.x,texCoordBottomRight.y),
                                    color));
                                    
    BatchItemDetails& details = registered ? _registeredQuad3DTextures[id] : _quad3DTextures[key];

    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        Vec3PairVec4Pair& lastKey = _lastRegisteredQuad3DTexture[id];
        if (lastKey != key) {
            details.clear();
            _lastRegisteredQuad3DTexture[id] = key;
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderQuad() 3D+texture ... RELEASING REGISTERED";
            #endif // def WANT_DEBUG
        }
        #ifdef WANT_DEBUG
        else {
            qCDebug(renderutils) << "renderQuad() 3D+texture ... REUSING PREVIOUSLY REGISTERED";
        }
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 3 + 3 + 2; // vertices + normals + tex coords
    const int VERTICES = 4; // 1 quad = 4 vertices
    const int NUM_POS_COORDS = 3;
    const int NUM_NORMAL_COORDS = 3;
    const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);
    const int VERTEX_TEXCOORD_OFFSET = VERTEX_NORMAL_OFFSET + NUM_NORMAL_COORDS * sizeof(float);


    if (!details.isCreated) {

        details.isCreated = true;
        details.vertices = VERTICES;
        details.vertexSize = FLOATS_PER_VERTEX; // NOTE: this isn't used for BatchItemDetails maybe we can get rid of it
        
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        auto streamFormat = std::make_shared<gpu::Stream::Format>();
        auto stream = std::make_shared<gpu::BufferStream>();

        details.verticesBuffer = verticesBuffer;
        details.colorBuffer = colorBuffer;
        details.streamFormat = streamFormat;
        details.stream = stream;
    
        details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), VERTEX_TEXCOORD_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
        float vertexBuffer[VERTICES * FLOATS_PER_VERTEX] = {
            bottomLeft.x, bottomLeft.y, bottomLeft.z, NORMAL.x, NORMAL.y, NORMAL.z, texCoordBottomLeft.x, texCoordBottomLeft.y,
            bottomRight.x, bottomRight.y, bottomRight.z, NORMAL.x, NORMAL.y, NORMAL.z, texCoordBottomRight.x, texCoordBottomRight.y,
            topLeft.x, topLeft.y, topLeft.z, NORMAL.x, NORMAL.y, NORMAL.z, texCoordTopLeft.x, texCoordTopLeft.y,
            topRight.x, topRight.y, topRight.z, NORMAL.x, NORMAL.y, NORMAL.z, texCoordTopRight.x, texCoordTopRight.y,
        };

        const int NUM_COLOR_SCALARS_PER_QUAD = 4;
        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);
        int colors[NUM_COLOR_SCALARS_PER_QUAD] = { compactColor, compactColor, compactColor, compactColor };

        details.verticesBuffer->append(sizeof(vertexBuffer), (gpu::Byte*) vertexBuffer);
        details.colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);
    }

    batch.setInputFormat(details.streamFormat);
    batch.setInputStream(0, *details.stream);
    batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
}

void GeometryCache::renderDashedLine(gpu::Batch& batch, const glm::vec3& start, const glm::vec3& end, const glm::vec4& color,
                                     const float dash_length, const float gap_length, int id) {

    bool registered = (id != UNKNOWN_ID);
    Vec3PairVec2Pair key(Vec3Pair(start, end), Vec2Pair(glm::vec2(color.x, color.y), glm::vec2(color.z, color.w)));
    BatchItemDetails& details = registered ? _registeredDashedLines[id] : _dashedLines[key];

    // if this is a registered , and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        if (_lastRegisteredDashedLines[id] != key) {
            details.clear();
            _lastRegisteredDashedLines[id] = key;
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderDashedLine()... RELEASING REGISTERED";
            #endif // def WANT_DEBUG
        }
    }

    if (!details.isCreated) {

        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                           ((int(color.y * 255.0f) & 0xFF) << 8) |
                           ((int(color.z * 255.0f) & 0xFF) << 16) |
                           ((int(color.w * 255.0f) & 0xFF) << 24);

        // draw each line segment with appropriate gaps
        const float SEGMENT_LENGTH = dash_length + gap_length;
        float length = glm::distance(start, end);
        float segmentCount = length / SEGMENT_LENGTH;
        int segmentCountFloor = (int)glm::floor(segmentCount);

        glm::vec3 segmentVector = (end - start) / segmentCount;
        glm::vec3 dashVector = segmentVector / SEGMENT_LENGTH * dash_length;
        glm::vec3 gapVector = segmentVector / SEGMENT_LENGTH * gap_length;

        const int FLOATS_PER_VERTEX = 3 + 3; // vertices + normals
        const int NUM_POS_COORDS = 3;
        const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);
        details.vertices = (segmentCountFloor + 1) * 2;
        details.vertexSize = FLOATS_PER_VERTEX;
        details.isCreated = true;
        
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        auto streamFormat = std::make_shared<gpu::Stream::Format>();
        auto stream = std::make_shared<gpu::BufferStream>();

        details.verticesBuffer = verticesBuffer;
        details.colorBuffer = colorBuffer;
        details.streamFormat = streamFormat;
        details.stream = stream;

        details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);

        int* colorData = new int[details.vertices];
        int* colorDataAt = colorData;

        float* vertexData = new float[details.vertices * FLOATS_PER_VERTEX];
        float* vertex = vertexData;

        const glm::vec3 NORMAL(1.0f, 0.0f, 0.0f);
        glm::vec3 point = start;
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        *(vertex++) = point.z;
        *(vertex++) = NORMAL.x;
        *(vertex++) = NORMAL.y;
        *(vertex++) = NORMAL.z;
        *(colorDataAt++) = compactColor;

        for (int i = 0; i < segmentCountFloor; i++) {
            point += dashVector;
            *(vertex++) = point.x;
            *(vertex++) = point.y;
            *(vertex++) = point.z;
            *(vertex++) = NORMAL.x;
            *(vertex++) = NORMAL.y;
            *(vertex++) = NORMAL.z;
            *(colorDataAt++) = compactColor;

            point += gapVector;
            *(vertex++) = point.x;
            *(vertex++) = point.y;
            *(vertex++) = point.z;
            *(vertex++) = NORMAL.x;
            *(vertex++) = NORMAL.y;
            *(vertex++) = NORMAL.z;
            *(colorDataAt++) = compactColor;
        }
        *(vertex++) = end.x;
        *(vertex++) = end.y;
        *(vertex++) = end.z;
        *(vertex++) = NORMAL.x;
        *(vertex++) = NORMAL.y;
        *(vertex++) = NORMAL.z;
        *(colorDataAt++) = compactColor;

        details.verticesBuffer->append(sizeof(float) * FLOATS_PER_VERTEX * details.vertices, (gpu::Byte*) vertexData);
        details.colorBuffer->append(sizeof(int) * details.vertices, (gpu::Byte*) colorData);
        delete[] vertexData;
        delete[] colorData;

        #ifdef WANT_DEBUG
        if (registered) {
            qCDebug(renderutils) << "new registered dashed line buffer made -- _registeredVertices:" << _registeredDashedLines.size();
        } else {
            qCDebug(renderutils) << "new dashed lines buffer made -- _dashedLines:" << _dashedLines.size();
        }
        #endif
    }

    batch.setInputFormat(details.streamFormat);
    batch.setInputStream(0, *details.stream);
    batch.draw(gpu::LINES, details.vertices, 0);
}


int GeometryCache::BatchItemDetails::population = 0;

GeometryCache::BatchItemDetails::BatchItemDetails() :
    verticesBuffer(NULL),
    colorBuffer(NULL),
    streamFormat(NULL),
    stream(NULL),
    vertices(0),
    vertexSize(0),
    isCreated(false)
{
    population++;
    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "BatchItemDetails()... population:" << population << "**********************************";
    #endif
}

GeometryCache::BatchItemDetails::BatchItemDetails(const GeometryCache::BatchItemDetails& other) :
    verticesBuffer(other.verticesBuffer),
    colorBuffer(other.colorBuffer),
    streamFormat(other.streamFormat),
    stream(other.stream),
    vertices(other.vertices),
    vertexSize(other.vertexSize),
    isCreated(other.isCreated)
{
    population++;
    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "BatchItemDetails()... population:" << population << "**********************************";
    #endif
}

GeometryCache::BatchItemDetails::~BatchItemDetails() {
    population--;
    clear(); 
    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "~BatchItemDetails()... population:" << population << "**********************************";
    #endif
}

void GeometryCache::BatchItemDetails::clear() {
    isCreated = false;
    verticesBuffer.reset();
    colorBuffer.reset();
    streamFormat.reset();
    stream.reset();
}

void GeometryCache::renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                               const glm::vec4& color1, const glm::vec4& color2, int id) {
                               
    bool registered = (id != UNKNOWN_ID);
    Vec3Pair key(p1, p2);

    BatchItemDetails& details = registered ? _registeredLine3DVBOs[id] : _line3DVBOs[key];

    int compactColor1 = ((int(color1.x * 255.0f) & 0xFF)) |
                        ((int(color1.y * 255.0f) & 0xFF) << 8) |
                        ((int(color1.z * 255.0f) & 0xFF) << 16) |
                        ((int(color1.w * 255.0f) & 0xFF) << 24);

    int compactColor2 = ((int(color2.x * 255.0f) & 0xFF)) |
                        ((int(color2.y * 255.0f) & 0xFF) << 8) |
                        ((int(color2.z * 255.0f) & 0xFF) << 16) |
                        ((int(color2.w * 255.0f) & 0xFF) << 24);


    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        Vec3Pair& lastKey = _lastRegisteredLine3D[id];
        if (lastKey != key) {
            details.clear();
            _lastRegisteredLine3D[id] = key;  
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderLine() 3D ... RELEASING REGISTERED line";
            #endif // def WANT_DEBUG
        }
        #ifdef WANT_DEBUG
        else {
            qCDebug(renderutils) << "renderLine() 3D ... REUSING PREVIOUSLY REGISTERED line";
        }
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 3 + 3; // vertices + normals
    const int NUM_POS_COORDS = 3;
    const int VERTEX_NORMAL_OFFSET = NUM_POS_COORDS * sizeof(float);
    const int vertices = 2;
    if (!details.isCreated) {

        details.isCreated = true;
        details.vertices = vertices;
        details.vertexSize = FLOATS_PER_VERTEX;
        
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        auto streamFormat = std::make_shared<gpu::Stream::Format>();
        auto stream = std::make_shared<gpu::BufferStream>();

        details.verticesBuffer = verticesBuffer;
        details.colorBuffer = colorBuffer;
        details.streamFormat = streamFormat;
        details.stream = stream;
    
        details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        details.streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_NORMAL_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);

        const glm::vec3 NORMAL(1.0f, 0.0f, 0.0f);
        float vertexBuffer[vertices * FLOATS_PER_VERTEX] = {
            p1.x, p1.y, p1.z, NORMAL.x, NORMAL.y, NORMAL.z,
            p2.x, p2.y, p2.z, NORMAL.x, NORMAL.y, NORMAL.z};

        const int NUM_COLOR_SCALARS = 2;
        int colors[NUM_COLOR_SCALARS] = { compactColor1, compactColor2 };

        details.verticesBuffer->append(sizeof(vertexBuffer), (gpu::Byte*) vertexBuffer);
        details.colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);

        #ifdef WANT_DEBUG
            if (id == UNKNOWN_ID) {
                qCDebug(renderutils) << "new renderLine() 3D VBO made -- _line3DVBOs.size():" << _line3DVBOs.size();
            } else {
                qCDebug(renderutils) << "new registered renderLine() 3D VBO made -- _registeredLine3DVBOs.size():" << _registeredLine3DVBOs.size();
            }
        #endif
    }

    // this is what it takes to render a quad
    batch.setInputFormat(details.streamFormat);
    batch.setInputStream(0, *details.stream);
    batch.draw(gpu::LINES, 2, 0);
}

void GeometryCache::renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2,                                
                                const glm::vec4& color1, const glm::vec4& color2, int id) {
                               
    bool registered = (id != UNKNOWN_ID);
    Vec2Pair key(p1, p2);

    BatchItemDetails& details = registered ? _registeredLine2DVBOs[id] : _line2DVBOs[key];

    int compactColor1 = ((int(color1.x * 255.0f) & 0xFF)) |
                        ((int(color1.y * 255.0f) & 0xFF) << 8) |
                        ((int(color1.z * 255.0f) & 0xFF) << 16) |
                        ((int(color1.w * 255.0f) & 0xFF) << 24);

    int compactColor2 = ((int(color2.x * 255.0f) & 0xFF)) |
                        ((int(color2.y * 255.0f) & 0xFF) << 8) |
                        ((int(color2.z * 255.0f) & 0xFF) << 16) |
                        ((int(color2.w * 255.0f) & 0xFF) << 24);


    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        Vec2Pair& lastKey = _lastRegisteredLine2D[id];
        if (lastKey != key) {
            details.clear();
            _lastRegisteredLine2D[id] = key;
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderLine() 2D ... RELEASING REGISTERED line";
            #endif // def WANT_DEBUG
        }
        #ifdef WANT_DEBUG
        else {
            qCDebug(renderutils) << "renderLine() 2D ... REUSING PREVIOUSLY REGISTERED line";
        }
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 2;
    const int vertices = 2;
    if (!details.isCreated) {

        details.isCreated = true;
        details.vertices = vertices;
        details.vertexSize = FLOATS_PER_VERTEX;
        
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        auto streamFormat = std::make_shared<gpu::Stream::Format>();
        auto stream = std::make_shared<gpu::BufferStream>();

        details.verticesBuffer = verticesBuffer;
        details.colorBuffer = colorBuffer;
        details.streamFormat = streamFormat;
        details.stream = stream;
    
        details.streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        float vertexBuffer[vertices * FLOATS_PER_VERTEX] = { p1.x, p1.y, p2.x, p2.y };

        const int NUM_COLOR_SCALARS = 2;
        int colors[NUM_COLOR_SCALARS] = { compactColor1, compactColor2 };

        details.verticesBuffer->append(sizeof(vertexBuffer), (gpu::Byte*) vertexBuffer);
        details.colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);

        #ifdef WANT_DEBUG
            if (id == UNKNOWN_ID) {
                qCDebug(renderutils) << "new renderLine() 2D VBO made -- _line3DVBOs.size():" << _line2DVBOs.size();
            } else {
                qCDebug(renderutils) << "new registered renderLine() 2D VBO made -- _registeredLine2DVBOs.size():" << _registeredLine2DVBOs.size();
            }
        #endif
    }

    // this is what it takes to render a quad
    batch.setInputFormat(details.streamFormat);
    batch.setInputStream(0, *details.stream);
    batch.draw(gpu::LINES, 2, 0);
}

void GeometryCache::useSimpleDrawPipeline(gpu::Batch& batch, bool noBlend) {
    if (!_standardDrawPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(standardTransformPNTC_vert));
        auto ps = gpu::Shader::createPixel(std::string(standardDrawTexture_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram((*program));

        auto state = std::make_shared<gpu::State>();

        // enable decal blend
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);

        _standardDrawPipeline = gpu::Pipeline::create(program, state);


        auto stateNoBlend = std::make_shared<gpu::State>();
        auto noBlendPS = gpu::StandardShaderLib::getDrawTextureOpaquePS();
        auto programNoBlend = gpu::Shader::createProgram(vs, noBlendPS);
        gpu::Shader::makeProgram((*programNoBlend));
        _standardDrawPipelineNoBlend = gpu::Pipeline::create(programNoBlend, stateNoBlend);
    }
    if (noBlend) {
        batch.setPipeline(_standardDrawPipelineNoBlend);
    } else {
        batch.setPipeline(_standardDrawPipeline);
    }
}

void GeometryCache::useGridPipeline(gpu::Batch& batch, GridBuffer gridBuffer, bool isLayered) {
    if (!_gridPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(standardTransformPNTC_vert));
        auto ps = gpu::Shader::createPixel(std::string(grid_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram((*program));
        _gridSlot = program->getBuffers().findLocation("gridBuffer");

        auto stateLayered = std::make_shared<gpu::State>();
        stateLayered->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _gridPipelineLayered = gpu::Pipeline::create(program, stateLayered);

        auto state = std::make_shared<gpu::State>(stateLayered->getValues());
        const float DEPTH_BIAS = 0.001f;
        state->setDepthBias(DEPTH_BIAS);
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        _gridPipeline = gpu::Pipeline::create(program, state);
    }

    gpu::PipelinePointer pipeline = isLayered ? _gridPipelineLayered : _gridPipeline;
    batch.setPipeline(pipeline);
    batch.setUniformBuffer(_gridSlot, gridBuffer);
}



class SimpleProgramKey {
public:
    enum FlagBit {
        IS_TEXTURED_FLAG = 0,
        IS_CULLED_FLAG,
        IS_EMISSIVE_FLAG,
        HAS_DEPTH_BIAS_FLAG,

        NUM_FLAGS,
    };

    enum Flag {
        IS_TEXTURED = (1 << IS_TEXTURED_FLAG),
        IS_CULLED = (1 << IS_CULLED_FLAG),
        IS_EMISSIVE = (1 << IS_EMISSIVE_FLAG),
        HAS_DEPTH_BIAS = (1 << HAS_DEPTH_BIAS_FLAG),
    };
    typedef unsigned short Flags;

    bool isFlag(short flagNum) const { return bool((_flags & flagNum) != 0); }

    bool isTextured() const { return isFlag(IS_TEXTURED); }
    bool isCulled() const { return isFlag(IS_CULLED); }
    bool isEmissive() const { return isFlag(IS_EMISSIVE); }
    bool hasDepthBias() const { return isFlag(HAS_DEPTH_BIAS); }

    Flags _flags = 0;
    short _spare = 0;

    int getRaw() const { return *reinterpret_cast<const int*>(this); }


    SimpleProgramKey(bool textured = false, bool culled = true,
                     bool emissive = false, bool depthBias = false) {
        _flags = (textured ? IS_TEXTURED : 0) | (culled ? IS_CULLED : 0) |
        (emissive ? IS_EMISSIVE : 0) | (depthBias ? HAS_DEPTH_BIAS : 0);
    }

    SimpleProgramKey(int bitmask) : _flags(bitmask) {}
};

inline uint qHash(const SimpleProgramKey& key, uint seed) {
    return qHash(key.getRaw(), seed);
}

inline bool operator==(const SimpleProgramKey& a, const SimpleProgramKey& b) {
    return a.getRaw() == b.getRaw();
}

void GeometryCache::bindSimpleProgram(gpu::Batch& batch, bool textured, bool culled, bool emissive, bool depthBiased) {
    batch.setPipeline(getSimplePipeline(textured, culled, emissive, depthBiased));

    // If not textured, set a default albedo map
    if (!textured) {
        batch.setResourceTexture(render::ShapePipeline::Slot::ALBEDO_MAP,
            DependencyManager::get<TextureCache>()->getWhiteTexture());
    }
    // Set a default normal map
    batch.setResourceTexture(render::ShapePipeline::Slot::NORMAL_FITTING_MAP,
        DependencyManager::get<TextureCache>()->getNormalFittingTexture());
}

gpu::PipelinePointer GeometryCache::getSimplePipeline(bool textured, bool culled, bool emissive, bool depthBiased) {
    SimpleProgramKey config{textured, culled, emissive, depthBiased};

    // Compile the shaders
    static std::once_flag once;
    std::call_once(once, [&]() {
        auto VS = gpu::Shader::createVertex(std::string(simple_vert));
        auto PS = gpu::Shader::createPixel(std::string(simple_textured_frag));
        auto PSEmissive = gpu::Shader::createPixel(std::string(simple_textured_emisive_frag));
        
        _simpleShader = gpu::Shader::createProgram(VS, PS);
        _emissiveShader = gpu::Shader::createProgram(VS, PSEmissive);
        
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), render::ShapePipeline::Slot::NORMAL_FITTING_MAP));
        gpu::Shader::makeProgram(*_simpleShader, slotBindings);
        gpu::Shader::makeProgram(*_emissiveShader, slotBindings);
    });

    // If the pipeline already exists, return it
    auto it = _simplePrograms.find(config);
    if (it != _simplePrograms.end()) {
        return it.value();
    }

    // If the pipeline did not exist, make it
    auto state = std::make_shared<gpu::State>();
    if (config.isCulled()) {
        state->setCullMode(gpu::State::CULL_BACK);
    } else {
        state->setCullMode(gpu::State::CULL_NONE);
    }
    state->setDepthTest(true, true, gpu::LESS_EQUAL);
    if (config.hasDepthBias()) {
        state->setDepthBias(1.0f);
        state->setDepthBiasSlopeScale(1.0f);
    }
    state->setBlendFunction(false,
        gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    gpu::ShaderPointer program = (config.isEmissive()) ? _emissiveShader : _simpleShader;
    gpu::PipelinePointer pipeline = gpu::Pipeline::create(program, state);
    _simplePrograms.insert(config, pipeline);
    return pipeline;
}

uint32_t toCompactColor(const glm::vec4& color) {
    uint32_t compactColor = ((int(color.x * 255.0f) & 0xFF)) |
    ((int(color.y * 255.0f) & 0xFF) << 8) |
    ((int(color.z * 255.0f) & 0xFF) << 16) |
    ((int(color.w * 255.0f) & 0xFF) << 24);
    return compactColor;
}

static const size_t INSTANCE_COLOR_BUFFER = 0;

void renderInstances(const std::string& name, gpu::Batch& batch, const glm::vec4& color, bool isWire,
                    const render::ShapePipelinePointer& pipeline, GeometryCache::Shape shape) {
    // Add pipeline to name
    std::string instanceName = name + std::to_string(std::hash<render::ShapePipelinePointer>()(pipeline));

    // Add color to named buffer
    {
        gpu::BufferPointer instanceColorBuffer = batch.getNamedBuffer(instanceName, INSTANCE_COLOR_BUFFER);
        auto compactColor = toCompactColor(color);
        instanceColorBuffer->append(compactColor);
    }

    // Add call to named buffer
    batch.setupNamedCalls(instanceName, [isWire, pipeline, shape](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
        batch.setPipeline(pipeline->pipeline);
        pipeline->prepare(batch);

        if (isWire) {
            DependencyManager::get<GeometryCache>()->renderWireShapeInstances(batch, shape, data.count(), data.buffers[INSTANCE_COLOR_BUFFER]);
        } else {
            DependencyManager::get<GeometryCache>()->renderShapeInstances(batch, shape, data.count(), data.buffers[INSTANCE_COLOR_BUFFER]);
        }
    });
}

void GeometryCache::renderSolidSphereInstance(gpu::Batch& batch, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    static const std::string INSTANCE_NAME = __FUNCTION__;
    renderInstances(INSTANCE_NAME, batch, color, false, pipeline, GeometryCache::Sphere);
}

void GeometryCache::renderWireSphereInstance(gpu::Batch& batch, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    static const std::string INSTANCE_NAME = __FUNCTION__;
    renderInstances(INSTANCE_NAME, batch, color, true, pipeline, GeometryCache::Sphere);
}

// Enable this in a debug build to cause 'box' entities to iterate through all the
// available shape types, both solid and wireframes
//#define DEBUG_SHAPES

void GeometryCache::renderSolidCubeInstance(gpu::Batch& batch, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    static const std::string INSTANCE_NAME = __FUNCTION__;
    
#ifdef DEBUG_SHAPES
    static auto startTime = usecTimestampNow();
    renderInstances(INSTANCE_NAME, batch, color, pipeline, [](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
        
        auto usecs = usecTimestampNow();
        usecs -= startTime;
        auto msecs = usecs / USECS_PER_MSEC;
        float seconds = msecs;
        seconds /= MSECS_PER_SECOND;
        float fractionalSeconds = seconds - floor(seconds);
        int shapeIndex = (int)seconds;
        
        // Every second we flip to the next shape.
        static const int SHAPE_COUNT = 5;
        GeometryCache::Shape shapes[SHAPE_COUNT] = {
            GeometryCache::Cube,
            GeometryCache::Tetrahedron,
            GeometryCache::Sphere,
            GeometryCache::Icosahedron,
            GeometryCache::Line,
        };
        
        shapeIndex %= SHAPE_COUNT;
        GeometryCache::Shape shape = shapes[shapeIndex];
        
        // For the first half second for a given shape, show the wireframe, for the second half, show the solid.
        if (fractionalSeconds > 0.5f) {
            renderInstances(INSTANCE_NAME, batch, color, true, pipeline, shape);
        } else {
            renderInstances(INSTANCE_NAME, batch, color, false, pipeline, shape);
        }
    });
#else
    renderInstances(INSTANCE_NAME, batch, color, false, pipeline, GeometryCache::Cube);
#endif
}

void GeometryCache::renderWireCubeInstance(gpu::Batch& batch, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    static const std::string INSTANCE_NAME = __FUNCTION__;
    renderInstances(INSTANCE_NAME, batch, color, true, pipeline, GeometryCache::Cube);
}
