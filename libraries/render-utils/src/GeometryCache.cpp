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

#include <gpu/GLBackend.h>

#include "TextureCache.h"
#include "RenderUtilsLogging.h"

#include "standardTransformPNTC_vert.h"
#include "standardDrawTexture_frag.h"

#include "gpu/StandardShaderLib.h"

#include "model/TextureMap.h"

//#define WANT_DEBUG

const int GeometryCache::UNKNOWN_ID = -1;


static const int VERTICES_PER_TRIANGLE = 3;

//static const uint FLOATS_PER_VERTEX = 3;
//static const uint TRIANGLES_PER_QUAD = 2;
//static const uint CUBE_FACES = 6;
//static const uint CUBE_VERTICES_PER_FACE = 4;
//static const uint CUBE_VERTICES = CUBE_FACES * CUBE_VERTICES_PER_FACE;
//static const uint CUBE_VERTEX_POINTS = CUBE_VERTICES * FLOATS_PER_VERTEX;
//static const uint CUBE_INDICES = CUBE_FACES * TRIANGLES_PER_QUAD * VERTICES_PER_TRIANGLE;
//static const uint SPHERE_LATITUDES = 24;
//static const uint SPHERE_MERIDIANS = SPHERE_LATITUDES * 2;
//static const uint SPHERE_INDICES = SPHERE_MERIDIANS * (SPHERE_LATITUDES - 1) * TRIANGLES_PER_QUAD * VERTICES_PER_TRIANGLE;

static const gpu::Element POSITION_ELEMENT{ gpu::VEC3, gpu::FLOAT, gpu::XYZ };
static const gpu::Element NORMAL_ELEMENT{ gpu::VEC3, gpu::FLOAT, gpu::XYZ };
static const gpu::Element COLOR_ELEMENT{ gpu::VEC4, gpu::NUINT8, gpu::RGBA };
static const gpu::Element TRANSFORM_ELEMENT{ gpu::MAT4, gpu::FLOAT, gpu::XYZW };

static gpu::Stream::FormatPointer SOLID_STREAM_FORMAT;
static gpu::Stream::FormatPointer INSTANCED_SOLID_STREAM_FORMAT;

static const uint SHAPE_VERTEX_STRIDE = sizeof(glm::vec3) * 2; // vertices and normals
static const uint SHAPE_NORMALS_OFFSET = sizeof(glm::vec3);


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
        _indexOffset = indexBuffer->getSize();
        _indexCount = indices.size();
        indexBuffer->append(indices);
    }

    if (!wireIndices.empty()) {
        _wireIndexOffset = indexBuffer->getSize();
        _wireIndexCount = wireIndices.size();
        indexBuffer->append(wireIndices);
    }
}

void GeometryCache::ShapeData::setupBatch(gpu::Batch& batch) const {
    batch.setInputBuffer(gpu::Stream::POSITION, _positionView);
    batch.setInputBuffer(gpu::Stream::NORMAL, _normalView);
}

void GeometryCache::ShapeData::draw(gpu::Batch& batch) const {
    if (_indexCount) {
        setupBatch(batch);
        batch.setIndexBuffer(gpu::UINT16, _indices, _indexOffset);
        batch.drawIndexed(gpu::TRIANGLES, _indexCount);
    }
}

void GeometryCache::ShapeData::drawWire(gpu::Batch& batch) const {
    if (_wireIndexCount) {
        setupBatch(batch);
        batch.setIndexBuffer(gpu::UINT16, _indices, _wireIndexOffset);
        batch.drawIndexed(gpu::LINES, _wireIndexCount);
    }
}

void GeometryCache::ShapeData::drawInstances(gpu::Batch& batch, size_t count) const {
    if (_indexCount) {
        setupBatch(batch);
        batch.setIndexBuffer(gpu::UINT16, _indices, _indexOffset);
        batch.drawIndexedInstanced(count, gpu::TRIANGLES, _indexCount);
    }
}

void GeometryCache::ShapeData::drawWireInstances(gpu::Batch& batch, size_t count) const {
    if (_wireIndexCount) {
        setupBatch(batch);
        batch.setIndexBuffer(gpu::UINT16, _indices, _wireIndexOffset);
        batch.drawIndexedInstanced(count, gpu::LINES, _wireIndexCount);
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

// FIXME solids need per-face vertices, but smooth shaded
// components do not.  Find a way to support using draw elements
// or draw arrays as appropriate
// Maybe special case cone and cylinder since they combine flat
// and smooth shading
void GeometryCache::buildShapes() {
    auto vertexBuffer = std::make_shared<gpu::Buffer>();
    auto indexBuffer = std::make_shared<gpu::Buffer>();
    uint16_t startingIndex = 0;
    
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
            index += startingIndex;
        }

        IndexVector wireIndices{
            0, 1, 1, 2, 2, 3, 3, 0, // front
            20, 21, 21, 22, 22, 23, 23, 20, // back
            0, 23, 1, 22, 2, 21, 3, 20 // sides
        };
        for (unsigned int i = 0; i < wireIndices.size(); ++i) {
            indices[i] += startingIndex;
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
                indices.push_back(vertexIndex + startingIndex);
            }
        }

        IndexVector wireIndices{
            0, 1, 1, 2, 2, 0,
            0, 3, 1, 3, 2, 3,
        };

        for (unsigned int i = 0; i < wireIndices.size(); ++i) {
            wireIndices[i] += startingIndex;
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
                    indices.push_back(vertexIndex + startingIndex);
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
                    indices.push_back(vertexIndex + startingIndex);
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
        wireIndices.push_back(0 + startingIndex);
        wireIndices.push_back(1 + startingIndex);

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
        INSTANCED_SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::INSTANCE_XFM, gpu::Stream::INSTANCE_XFM, TRANSFORM_ELEMENT, 0, gpu::Stream::PER_INSTANCE);
    }
    return INSTANCED_SOLID_STREAM_FORMAT;
}

GeometryCache::GeometryCache() :
    _nextID(0)
{
    buildShapes();
}

GeometryCache::~GeometryCache() {
    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "GeometryCache::~GeometryCache()... ";
        qCDebug(renderutils) << "    _registeredLine3DVBOs.size():" << _registeredLine3DVBOs.size();
        qCDebug(renderutils) << "    _line3DVBOs.size():" << _line3DVBOs.size();
        qCDebug(renderutils) << "    BatchItemDetails... population:" << GeometryCache::BatchItemDetails::population;
    #endif //def WANT_DEBUG
}

void setupBatchInstance(gpu::Batch& batch, gpu::BufferPointer transformBuffer, gpu::BufferPointer colorBuffer) {
    gpu::BufferView colorView(colorBuffer, COLOR_ELEMENT);
    batch.setInputBuffer(gpu::Stream::COLOR, colorView);
    gpu::BufferView instanceXfmView(transformBuffer, 0, transformBuffer->getSize(), TRANSFORM_ELEMENT);
    batch.setInputBuffer(gpu::Stream::INSTANCE_XFM, instanceXfmView);
}

void GeometryCache::renderShape(gpu::Batch& batch, Shape shape) {
    batch.setInputFormat(getSolidStreamFormat());
    _shapes[shape].draw(batch);
}

void GeometryCache::renderWireShape(gpu::Batch& batch, Shape shape) {
    batch.setInputFormat(getSolidStreamFormat());
    _shapes[shape].drawWire(batch);
}

void GeometryCache::renderShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& transformBuffer, gpu::BufferPointer& colorBuffer) {
    batch.setInputFormat(getInstancedSolidStreamFormat());
    setupBatchInstance(batch, transformBuffer, colorBuffer);
    _shapes[shape].drawInstances(batch, count);
}

void GeometryCache::renderWireShapeInstances(gpu::Batch& batch, Shape shape, size_t count, gpu::BufferPointer& transformBuffer, gpu::BufferPointer& colorBuffer) {
    batch.setInputFormat(getInstancedSolidStreamFormat());
    setupBatchInstance(batch, transformBuffer, colorBuffer);
    _shapes[shape].drawWireInstances(batch, count);
}

void GeometryCache::renderCubeInstances(gpu::Batch& batch, size_t count, gpu::BufferPointer transformBuffer, gpu::BufferPointer colorBuffer) {
    renderShapeInstances(batch, Cube, count, transformBuffer, colorBuffer);
}

void GeometryCache::renderWireCubeInstances(gpu::Batch& batch, size_t count, gpu::BufferPointer transformBuffer, gpu::BufferPointer colorBuffer) {
    renderWireShapeInstances(batch, Cube, count, transformBuffer, colorBuffer);
}

void GeometryCache::renderCube(gpu::Batch& batch) {
    renderShape(batch, Cube);
}

void GeometryCache::renderWireCube(gpu::Batch& batch) {
    renderWireShape(batch, Cube);
}

void GeometryCache::renderSphereInstances(gpu::Batch& batch, size_t count, gpu::BufferPointer transformBuffer, gpu::BufferPointer colorBuffer) {
    renderShapeInstances(batch, Sphere, count, transformBuffer, colorBuffer);
}

void GeometryCache::renderSphere(gpu::Batch& batch) {
    renderShape(batch, Sphere);
}

void GeometryCache::renderWireSphere(gpu::Batch& batch) {
    renderWireShape(batch, Sphere);
}


void GeometryCache::renderGrid(gpu::Batch& batch, int xDivisions, int yDivisions, const glm::vec4& color) {
    IntPair key(xDivisions, yDivisions);
    Vec3Pair colorKey(glm::vec3(color.x, color.y, yDivisions), glm::vec3(color.z, color.y, xDivisions));

    int vertices = (xDivisions + 1 + yDivisions + 1) * 2;
    if (!_gridBuffers.contains(key)) {
        auto verticesBuffer = std::make_shared<gpu::Buffer>();

        GLfloat* vertexData = new GLfloat[vertices * 2];
        GLfloat* vertex = vertexData;

        for (int i = 0; i <= xDivisions; i++) {
            float x = (float)i / xDivisions;
        
            *(vertex++) = x;
            *(vertex++) = 0.0f;
            
            *(vertex++) = x;
            *(vertex++) = 1.0f;
        }
        for (int i = 0; i <= yDivisions; i++) {
            float y = (float)i / yDivisions;
            
            *(vertex++) = 0.0f;
            *(vertex++) = y;
            
            *(vertex++) = 1.0f;
            *(vertex++) = y;
        }

        verticesBuffer->append(sizeof(GLfloat) * vertices * 2, (gpu::Byte*) vertexData);
        delete[] vertexData;
        
        _gridBuffers[key] = verticesBuffer;
    }

    if (!_gridColors.contains(colorKey)) {
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        _gridColors[colorKey] = colorBuffer;

        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);

        int* colorData = new int[vertices];
        int* colorDataAt = colorData;
                            
        for(int v = 0; v < vertices; v++) {
            *(colorDataAt++) = compactColor;
        }

        colorBuffer->append(sizeof(int) * vertices, (gpu::Byte*) colorData);
        delete[] colorData;
    }
    gpu::BufferPointer verticesBuffer = _gridBuffers[key];
    gpu::BufferPointer colorBuffer = _gridColors[colorKey];

    const int VERTICES_SLOT = 0;
    const int COLOR_SLOT = 1;
    auto streamFormat = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
    
    streamFormat->setAttribute(gpu::Stream::POSITION, VERTICES_SLOT, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
    streamFormat->setAttribute(gpu::Stream::COLOR, COLOR_SLOT, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

    gpu::BufferView verticesView(verticesBuffer, 0, verticesBuffer->getSize(), streamFormat->getAttributes().at(gpu::Stream::POSITION)._element);
    gpu::BufferView colorView(colorBuffer, streamFormat->getAttributes().at(gpu::Stream::COLOR)._element);
    
    batch.setInputFormat(streamFormat);
    batch.setInputBuffer(VERTICES_SLOT, verticesView);
    batch.setInputBuffer(COLOR_SLOT, colorView);
    batch.draw(gpu::LINES, vertices, 0);
}

// TODO: why do we seem to create extra BatchItemDetails when we resize the window?? what's that??
void GeometryCache::renderGrid(gpu::Batch& batch, int x, int y, int width, int height, int rows, int cols, const glm::vec4& color, int id) {
    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "GeometryCache::renderGrid(x["<<x<<"], "
            "y["<<y<<"],"
            "w["<<width<<"],"
            "h["<<height<<"],"
            "rows["<<rows<<"],"
            "cols["<<cols<<"],"
            " id:"<<id<<")...";
    #endif
            
    bool registered = (id != UNKNOWN_ID);
    Vec3Pair key(glm::vec3(x, y, width), glm::vec3(height, rows, cols));
    Vec3Pair colorKey(glm::vec3(color.x, color.y, rows), glm::vec3(color.z, color.y, cols));

    int vertices = (cols + 1 + rows + 1) * 2;
    if ((registered && (!_registeredAlternateGridBuffers.contains(id) || _lastRegisteredAlternateGridBuffers[id] != key))
        || (!registered && !_alternateGridBuffers.contains(key))) {

        if (registered && _registeredAlternateGridBuffers.contains(id)) {
            _registeredAlternateGridBuffers[id].reset();
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderGrid()... RELEASING REGISTERED VERTICES BUFFER";
            #endif
        }

        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        if (registered) {
            _registeredAlternateGridBuffers[id] = verticesBuffer;
            _lastRegisteredAlternateGridBuffers[id] = key;
        } else {
            _alternateGridBuffers[key] = verticesBuffer;
        }

        GLfloat* vertexData = new GLfloat[vertices * 2];
        GLfloat* vertex = vertexData;

        int dx = width / cols;
        int dy = height / rows;
        int tx = x;
        int ty = y;

        // Draw horizontal grid lines
        for (int i = rows + 1; --i >= 0; ) {
            *(vertex++) = x;
            *(vertex++) = ty;

            *(vertex++) = x + width;
            *(vertex++) = ty;

            ty += dy;
        }
        // Draw vertical grid lines
        for (int i = cols + 1; --i >= 0; ) {
            *(vertex++) = tx;
            *(vertex++) = y;

            *(vertex++) = tx;
            *(vertex++) = y + height;
            tx += dx;
        }

        verticesBuffer->append(sizeof(GLfloat) * vertices * 2, (gpu::Byte*) vertexData);
        delete[] vertexData;
    }

    if (!_gridColors.contains(colorKey)) {
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        _gridColors[colorKey] = colorBuffer;

        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);

        int* colorData = new int[vertices];
        int* colorDataAt = colorData;
                            
                            
        for(int v = 0; v < vertices; v++) {
            *(colorDataAt++) = compactColor;
        }

        colorBuffer->append(sizeof(int) * vertices, (gpu::Byte*) colorData);
        delete[] colorData;
    }
    gpu::BufferPointer verticesBuffer = registered ? _registeredAlternateGridBuffers[id] : _alternateGridBuffers[key];
    
    gpu::BufferPointer colorBuffer = _gridColors[colorKey];

    const int VERTICES_SLOT = 0;
    const int COLOR_SLOT = 1;
    auto streamFormat = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
    
    streamFormat->setAttribute(gpu::Stream::POSITION, VERTICES_SLOT, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
    streamFormat->setAttribute(gpu::Stream::COLOR, COLOR_SLOT, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

    gpu::BufferView verticesView(verticesBuffer, 0, verticesBuffer->getSize(), streamFormat->getAttributes().at(gpu::Stream::POSITION)._element);
    gpu::BufferView colorView(colorBuffer, streamFormat->getAttributes().at(gpu::Stream::COLOR)._element);

    batch.setInputFormat(streamFormat);
    batch.setInputBuffer(VERTICES_SLOT, verticesView);
    batch.setInputBuffer(COLOR_SLOT, colorView);
    batch.draw(gpu::LINES, vertices, 0);
}

void GeometryCache::updateVertices(int id, const QVector<glm::vec2>& points, const glm::vec4& color) {
    BatchItemDetails& details = _registeredVertices[id];

    if (details.isCreated) {
        details.clear();
        #ifdef WANT_DEBUG
            qCDebug(renderutils) << "updateVertices()... RELEASING REGISTERED";
        #endif // def WANT_DEBUG
    }

    const int FLOATS_PER_VERTEX = 2;
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
    details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

    details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
    details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);

    details.vertices = points.size();
    details.vertexSize = FLOATS_PER_VERTEX;

    int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                        ((int(color.y * 255.0f) & 0xFF) << 8) |
                        ((int(color.z * 255.0f) & 0xFF) << 16) |
                        ((int(color.w * 255.0f) & 0xFF) << 24);

    GLfloat* vertexData = new GLfloat[details.vertices * FLOATS_PER_VERTEX];
    GLfloat* vertex = vertexData;

    int* colorData = new int[details.vertices];
    int* colorDataAt = colorData;

    foreach (const glm::vec2& point, points) {
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        
        *(colorDataAt++) = compactColor;
    }

    details.verticesBuffer->append(sizeof(GLfloat) * FLOATS_PER_VERTEX * details.vertices, (gpu::Byte*) vertexData);
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

    const int FLOATS_PER_VERTEX = 3;
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
    details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

    details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
    details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);

    details.vertices = points.size();
    details.vertexSize = FLOATS_PER_VERTEX;

    int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                        ((int(color.y * 255.0f) & 0xFF) << 8) |
                        ((int(color.z * 255.0f) & 0xFF) << 16) |
                        ((int(color.w * 255.0f) & 0xFF) << 24);

    GLfloat* vertexData = new GLfloat[details.vertices * FLOATS_PER_VERTEX];
    GLfloat* vertex = vertexData;

    int* colorData = new int[details.vertices];
    int* colorDataAt = colorData;

    foreach (const glm::vec3& point, points) {
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        *(vertex++) = point.z;
        
        *(colorDataAt++) = compactColor;
    }

    details.verticesBuffer->append(sizeof(GLfloat) * FLOATS_PER_VERTEX * details.vertices, (gpu::Byte*) vertexData);
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

    const int FLOATS_PER_VERTEX = 5;
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
    details.streamFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), 3 * sizeof(float));
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

    GLfloat* vertexData = new GLfloat[details.vertices * FLOATS_PER_VERTEX];
    GLfloat* vertex = vertexData;

    int* colorData = new int[details.vertices];
    int* colorDataAt = colorData;

    for (int i = 0; i < points.size(); i++) {
        glm::vec3 point = points[i];
        glm::vec2 texCoord = texCoords[i];
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        *(vertex++) = point.z;
        *(vertex++) = texCoord.x;
        *(vertex++) = texCoord.y;

        *(colorDataAt++) = compactColor;
    }

    details.verticesBuffer->append(sizeof(GLfloat) * FLOATS_PER_VERTEX * details.vertices, (gpu::Byte*) vertexData);
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


        GLfloat vertexBuffer[NUM_FLOATS]; // only vertices, no normals because we're a 2D quad
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

    const int FLOATS_PER_VERTEX = 2; // vertices
    const int VERTICES = 4; // 1 quad = 4 vertices

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
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        float vertexBuffer[VERTICES * FLOATS_PER_VERTEX] = {    
                            minCorner.x, minCorner.y,
                            maxCorner.x, minCorner.y,
                            minCorner.x, maxCorner.y,
                            maxCorner.x, maxCorner.y,
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

    const int FLOATS_PER_VERTEX = 2 * 2; // text coords & vertices
    const int VERTICES = 4; // 1 quad = 4 vertices
    const int NUM_POS_COORDS = 2;
    const int VERTEX_TEXCOORD_OFFSET = NUM_POS_COORDS * sizeof(float);

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
        details.streamFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), VERTEX_TEXCOORD_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        float vertexBuffer[VERTICES * FLOATS_PER_VERTEX] = {    
                                                        minCorner.x, minCorner.y, texCoordMinCorner.x, texCoordMinCorner.y,
                                                        maxCorner.x, minCorner.y, texCoordMaxCorner.x, texCoordMinCorner.y,
                                                        minCorner.x, maxCorner.y, texCoordMinCorner.x, texCoordMaxCorner.y,
                                                        maxCorner.x, maxCorner.y, texCoordMaxCorner.x, texCoordMaxCorner.y,
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

    const int FLOATS_PER_VERTEX = 3; // vertices
    const int VERTICES = 4; // 1 quad = 4 vertices

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
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        float vertexBuffer[VERTICES * FLOATS_PER_VERTEX] = {    
                            minCorner.x, minCorner.y, minCorner.z,
                            maxCorner.x, minCorner.y, minCorner.z,
                            minCorner.x, maxCorner.y, maxCorner.z,
                            maxCorner.x, maxCorner.y, maxCorner.z,
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

    const int FLOATS_PER_VERTEX = 3 + 2; // 3d vertices + text coords
    const int VERTICES = 4; // 1 quad = 4 vertices
    const int NUM_POS_COORDS = 3;
    const int VERTEX_TEXCOORD_OFFSET = NUM_POS_COORDS * sizeof(float);

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
        details.streamFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), VERTEX_TEXCOORD_OFFSET);
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);


        float vertexBuffer[VERTICES * FLOATS_PER_VERTEX] = {
                                bottomLeft.x, bottomLeft.y, bottomLeft.z, texCoordBottomLeft.x, texCoordBottomLeft.y,
                                bottomRight.x, bottomRight.y, bottomRight.z, texCoordBottomRight.x, texCoordBottomRight.y,
                                topLeft.x, topLeft.y, topLeft.z, texCoordTopLeft.x, texCoordTopLeft.y,
                                topRight.x, topRight.y, topRight.z, texCoordTopRight.x, texCoordTopRight.y,
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

        const int FLOATS_PER_VERTEX = 3;
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
        details.streamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));

        details.stream->addBuffer(details.verticesBuffer, 0, details.streamFormat->getChannels().at(0)._stride);
        details.stream->addBuffer(details.colorBuffer, 0, details.streamFormat->getChannels().at(1)._stride);

        int* colorData = new int[details.vertices];
        int* colorDataAt = colorData;

        GLfloat* vertexData = new GLfloat[details.vertices * FLOATS_PER_VERTEX];
        GLfloat* vertex = vertexData;

        glm::vec3 point = start;
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        *(vertex++) = point.z;
        *(colorDataAt++) = compactColor;

        for (int i = 0; i < segmentCountFloor; i++) {
            point += dashVector;
            *(vertex++) = point.x;
            *(vertex++) = point.y;
            *(vertex++) = point.z;
            *(colorDataAt++) = compactColor;

            point += gapVector;
            *(vertex++) = point.x;
            *(vertex++) = point.y;
            *(vertex++) = point.z;
            *(colorDataAt++) = compactColor;
        }
        *(vertex++) = end.x;
        *(vertex++) = end.y;
        *(vertex++) = end.z;
        *(colorDataAt++) = compactColor;

        details.verticesBuffer->append(sizeof(GLfloat) * FLOATS_PER_VERTEX * details.vertices, (gpu::Byte*) vertexData);
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

    const int FLOATS_PER_VERTEX = 3;
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


        float vertexBuffer[vertices * FLOATS_PER_VERTEX] = { p1.x, p1.y, p1.z, p2.x, p2.y, p2.z };

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
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(standardTransformPNTC_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(standardDrawTexture_frag)));
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));
        gpu::Shader::makeProgram((*program));

        auto state = std::make_shared<gpu::State>();


        // enable decal blend
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);

        _standardDrawPipeline.reset(gpu::Pipeline::create(program, state));


        auto stateNoBlend = std::make_shared<gpu::State>();
        auto noBlendPS = gpu::StandardShaderLib::getDrawTextureOpaquePS();
        auto programNoBlend = gpu::ShaderPointer(gpu::Shader::createProgram(vs, noBlendPS));
        gpu::Shader::makeProgram((*programNoBlend));
        _standardDrawPipelineNoBlend.reset(gpu::Pipeline::create(programNoBlend, stateNoBlend));
    }
    if (noBlend) {
        batch.setPipeline(_standardDrawPipelineNoBlend);
    } else {
        batch.setPipeline(_standardDrawPipeline);
    }
}
