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

#include <qmath.h>
#include <cmath>

#include <QtCore/QThreadPool>
#include <QtCore/QFileInfo>
#include <QtNetwork/QNetworkReply>

#include <FSTReader.h>
#include <NumericalConstants.h>
#include <shared/Shapes.h>

#include "TextureCache.h"
#include "RenderUtilsLogging.h"

#include "gpu/StandardShaderLib.h"

#include "model/TextureMap.h"

#include "standardTransformPNTC_vert.h"
#include "standardDrawTexture_frag.h"

#include "simple_vert.h"
#include "simple_textured_frag.h"
#include "simple_textured_unlit_frag.h"
#include "simple_opaque_web_browser_frag.h"
#include "simple_transparent_web_browser_frag.h"
#include "glowLine_vert.h"
#include "glowLine_frag.h"

#include "grid_frag.h"

//#define WANT_DEBUG

const int GeometryCache::UNKNOWN_ID = -1;


static const int VERTICES_PER_TRIANGLE = 3;

static const gpu::Element POSITION_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
static const gpu::Element NORMAL_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
static const gpu::Element COLOR_ELEMENT { gpu::VEC4, gpu::NUINT8, gpu::RGBA };

static gpu::Stream::FormatPointer SOLID_STREAM_FORMAT;
static gpu::Stream::FormatPointer INSTANCED_SOLID_STREAM_FORMAT;

static const uint SHAPE_VERTEX_STRIDE = sizeof(glm::vec3) * 2; // vertices and normals
static const uint SHAPE_NORMALS_OFFSET = sizeof(glm::vec3);
static const gpu::Type SHAPE_INDEX_TYPE = gpu::UINT32;
static const uint SHAPE_INDEX_SIZE = sizeof(gpu::uint32);

template <size_t SIDES>
std::vector<vec3> polygon() {
    std::vector<vec3> result;
    result.reserve(SIDES);
    double angleIncrement = 2.0 * M_PI / SIDES;
    for (size_t i = 0; i < SIDES; ++i) {
        double angle = (double)i * angleIncrement;
        result.push_back(vec3{ cos(angle) * 0.5, 0.0, sin(angle) * 0.5 });
    }
    return result;
}

void GeometryCache::ShapeData::setupVertices(gpu::BufferPointer& vertexBuffer, const geometry::VertexVector& vertices) {
    vertexBuffer->append(vertices);

    _positionView = gpu::BufferView(vertexBuffer, 0,
        vertexBuffer->getSize(), SHAPE_VERTEX_STRIDE, POSITION_ELEMENT);
    _normalView = gpu::BufferView(vertexBuffer, SHAPE_NORMALS_OFFSET,
        vertexBuffer->getSize(), SHAPE_VERTEX_STRIDE, NORMAL_ELEMENT);
}

void GeometryCache::ShapeData::setupIndices(gpu::BufferPointer& indexBuffer, const geometry::IndexVector& indices, const geometry::IndexVector& wireIndices) {
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

static const size_t ICOSAHEDRON_TO_SPHERE_TESSELATION_COUNT = 3;

size_t GeometryCache::getShapeTriangleCount(Shape shape) {
    return _shapes[shape]._indexCount / VERTICES_PER_TRIANGLE;
}

size_t GeometryCache::getSphereTriangleCount() {
    return getShapeTriangleCount(Sphere);
}

size_t GeometryCache::getCubeTriangleCount() {
    return getShapeTriangleCount(Cube);
}

using IndexPair = uint64_t;
using IndexPairs = std::unordered_set<IndexPair>;

static IndexPair indexToken(geometry::Index a, geometry::Index b) {
    if (a > b) {
        std::swap(a, b);
    }
    return (((IndexPair)a) << 32) | ((IndexPair)b);
}

template <size_t N>
void setupFlatShape(GeometryCache::ShapeData& shapeData, const geometry::Solid<N>& shape, gpu::BufferPointer& vertexBuffer, gpu::BufferPointer& indexBuffer) {
    using namespace geometry;
    Index baseVertex = (Index)(vertexBuffer->getSize() / SHAPE_VERTEX_STRIDE);
    VertexVector vertices;
    IndexVector solidIndices, wireIndices;
    IndexPairs wireSeenIndices;

    size_t faceCount = shape.faces.size();
    size_t faceIndexCount = triangulatedFaceIndexCount<N>();

    vertices.reserve(N * faceCount * 2);
    solidIndices.reserve(faceIndexCount * faceCount);

    for (size_t f = 0; f < faceCount; ++f) {
        const Face<N>& face = shape.faces[f];
        // Compute the face normal
        vec3 faceNormal = shape.getFaceNormal(f);

        // Create the vertices for the face
        for (Index i = 0; i < N; ++i) {
            Index originalIndex = face[i];
            vertices.push_back(shape.vertices[originalIndex]);
            vertices.push_back(faceNormal);
        }

        // Create the wire indices for unseen edges
        for (Index i = 0; i < N; ++i) {
            Index a = i;
            Index b = (i + 1) % N;
            auto token = indexToken(face[a], face[b]);
            if (0 == wireSeenIndices.count(token)) {
                wireSeenIndices.insert(token);
                wireIndices.push_back(a + baseVertex);
                wireIndices.push_back(b + baseVertex);
            }
        }

        // Create the solid face indices
        for (Index i = 0; i < N - 2; ++i) {
            solidIndices.push_back(0 + baseVertex);
            solidIndices.push_back(i + 1 + baseVertex);
            solidIndices.push_back(i + 2 + baseVertex);
        }
        baseVertex += (Index)N;
    }

    shapeData.setupVertices(vertexBuffer, vertices);
    shapeData.setupIndices(indexBuffer, solidIndices, wireIndices);
}

template <size_t N>
void setupSmoothShape(GeometryCache::ShapeData& shapeData, const geometry::Solid<N>& shape, gpu::BufferPointer& vertexBuffer, gpu::BufferPointer& indexBuffer) {
    using namespace geometry;
    Index baseVertex = (Index)(vertexBuffer->getSize() / SHAPE_VERTEX_STRIDE);

    VertexVector vertices;
    vertices.reserve(shape.vertices.size() * 2);
    for (const auto& vertex : shape.vertices) {
        vertices.push_back(vertex);
        vertices.push_back(vertex);
    }

    IndexVector solidIndices, wireIndices;
    IndexPairs wireSeenIndices;

    size_t faceCount = shape.faces.size();
    size_t faceIndexCount = triangulatedFaceIndexCount<N>();

    solidIndices.reserve(faceIndexCount * faceCount);

    for (size_t f = 0; f < faceCount; ++f) {
        const Face<N>& face = shape.faces[f];
        // Create the wire indices for unseen edges
        for (Index i = 0; i < N; ++i) {
            Index a = face[i];
            Index b = face[(i + 1) % N];
            auto token = indexToken(a, b);
            if (0 == wireSeenIndices.count(token)) {
                wireSeenIndices.insert(token);
                wireIndices.push_back(a + baseVertex);
                wireIndices.push_back(b + baseVertex);
            }
        }

        // Create the solid face indices
        for (Index i = 0; i < N - 2; ++i) {
            solidIndices.push_back(face[i] + baseVertex);
            solidIndices.push_back(face[i + 1] + baseVertex);
            solidIndices.push_back(face[i + 2] + baseVertex);
        }
    }

    shapeData.setupVertices(vertexBuffer, vertices);
    shapeData.setupIndices(indexBuffer, solidIndices, wireIndices);
}

template <uint32_t N>
void extrudePolygon(GeometryCache::ShapeData& shapeData, gpu::BufferPointer& vertexBuffer, gpu::BufferPointer& indexBuffer) {
    using namespace geometry;
    Index baseVertex = (Index)(vertexBuffer->getSize() / SHAPE_VERTEX_STRIDE);
    VertexVector vertices;
    IndexVector solidIndices, wireIndices;

    // Top and bottom faces
    std::vector<vec3> shape = polygon<N>();
    for (const vec3& v : shape) {
        vertices.push_back(vec3(v.x, 0.5f, v.z));
        vertices.push_back(vec3(0, 1, 0));
    }
    for (const vec3& v : shape) {
        vertices.push_back(vec3(v.x, -0.5f, v.z));
        vertices.push_back(vec3(0, -1, 0));
    }
    for (uint32_t i = 2; i < N; ++i) {
        solidIndices.push_back(baseVertex + 0);
        solidIndices.push_back(baseVertex + i);
        solidIndices.push_back(baseVertex + i - 1);
        solidIndices.push_back(baseVertex + N);
        solidIndices.push_back(baseVertex + i + N - 1);
        solidIndices.push_back(baseVertex + i + N);
    }
    for (uint32_t i = 1; i <= N; ++i) {
        wireIndices.push_back(baseVertex + (i % N));
        wireIndices.push_back(baseVertex + i - 1);
        wireIndices.push_back(baseVertex + (i % N) + N);
        wireIndices.push_back(baseVertex + (i - 1) + N);
    }

    // Now do the sides
    baseVertex += 2 * N;

    for (uint32_t i = 0; i < N; ++i) {
        vec3 left = shape[i];
        vec3 right = shape[(i + 1) % N];
        vec3 normal = glm::normalize(left + right);
        vec3 topLeft = vec3(left.x, 0.5f, left.z);
        vec3 topRight = vec3(right.x, 0.5f, right.z);
        vec3 bottomLeft = vec3(left.x, -0.5f, left.z);
        vec3 bottomRight = vec3(right.x, -0.5f, right.z);

        vertices.push_back(topLeft);
        vertices.push_back(normal);
        vertices.push_back(bottomLeft);
        vertices.push_back(normal);
        vertices.push_back(topRight);
        vertices.push_back(normal);
        vertices.push_back(bottomRight);
        vertices.push_back(normal);

        solidIndices.push_back(baseVertex + 0);
        solidIndices.push_back(baseVertex + 2);
        solidIndices.push_back(baseVertex + 1);
        solidIndices.push_back(baseVertex + 1);
        solidIndices.push_back(baseVertex + 2);
        solidIndices.push_back(baseVertex + 3);
        wireIndices.push_back(baseVertex + 0);
        wireIndices.push_back(baseVertex + 1);
        wireIndices.push_back(baseVertex + 3);
        wireIndices.push_back(baseVertex + 2);
        baseVertex += 4;
    }

    shapeData.setupVertices(vertexBuffer, vertices);
    shapeData.setupIndices(indexBuffer, solidIndices, wireIndices);
}

// FIXME solids need per-face vertices, but smooth shaded
// components do not.  Find a way to support using draw elements
// or draw arrays as appropriate
// Maybe special case cone and cylinder since they combine flat
// and smooth shading
void GeometryCache::buildShapes() {
    using namespace geometry;
    auto vertexBuffer = std::make_shared<gpu::Buffer>();
    auto indexBuffer = std::make_shared<gpu::Buffer>();
    // Cube 
    setupFlatShape(_shapes[Cube], geometry::cube(), _shapeVertices, _shapeIndices);
    // Tetrahedron
    setupFlatShape(_shapes[Tetrahedron], geometry::tetrahedron(), _shapeVertices, _shapeIndices);
    // Icosahedron
    setupFlatShape(_shapes[Icosahedron], geometry::icosahedron(), _shapeVertices, _shapeIndices);
    // Octahedron
    setupFlatShape(_shapes[Octahedron], geometry::octahedron(), _shapeVertices, _shapeIndices);
    // Dodecahedron
    setupFlatShape(_shapes[Dodecahedron], geometry::dodecahedron(), _shapeVertices, _shapeIndices);

    // Sphere
    // FIXME this uses way more vertices than required.  Should find a way to calculate the indices
    // using shared vertices for better vertex caching
    auto sphere = geometry::tesselate(geometry::icosahedron(), ICOSAHEDRON_TO_SPHERE_TESSELATION_COUNT);
    sphere.fitDimension(1.0f);
    setupSmoothShape(_shapes[Sphere], sphere, _shapeVertices, _shapeIndices);

    // Line
    {
        Index baseVertex = (Index)(_shapeVertices->getSize() / SHAPE_VERTEX_STRIDE);
        ShapeData& shapeData = _shapes[Line];
        shapeData.setupVertices(_shapeVertices, VertexVector {
            vec3(-0.5, 0, 0), vec3(-0.5f, 0, 0),
            vec3(0.5f, 0, 0), vec3(0.5f, 0, 0)
        });
        IndexVector wireIndices;
        // Only two indices
        wireIndices.push_back(0 + baseVertex);
        wireIndices.push_back(1 + baseVertex);
        shapeData.setupIndices(_shapeIndices, IndexVector(), wireIndices);
    }

    // Not implememented yet:

    //Triangle,
    extrudePolygon<3>(_shapes[Triangle], _shapeVertices, _shapeIndices);
    //Hexagon,
    extrudePolygon<6>(_shapes[Hexagon], _shapeVertices, _shapeIndices);
    //Octagon,
    extrudePolygon<8>(_shapes[Octagon], _shapeVertices, _shapeIndices);

    //Quad,
    //Circle,
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

render::ShapePipelinePointer GeometryCache::_simpleOpaquePipeline;
render::ShapePipelinePointer GeometryCache::_simpleTransparentPipeline;
render::ShapePipelinePointer GeometryCache::_simpleWirePipeline;

GeometryCache::GeometryCache() :
_nextID(0) {
    buildShapes();
    GeometryCache::_simpleOpaquePipeline =
        std::make_shared<render::ShapePipeline>(getSimplePipeline(false, false, true, false), nullptr,
            [](const render::ShapePipeline&, gpu::Batch& batch) {
                // Set the defaults needed for a simple program
                batch.setResourceTexture(render::ShapePipeline::Slot::MAP::ALBEDO,
                    DependencyManager::get<TextureCache>()->getWhiteTexture());
                batch.setResourceTexture(render::ShapePipeline::Slot::MAP::NORMAL_FITTING,
                    DependencyManager::get<TextureCache>()->getNormalFittingTexture());
            }
        );
    GeometryCache::_simpleTransparentPipeline =
        std::make_shared<render::ShapePipeline>(getSimplePipeline(false, true, true, false), nullptr,
            [](const render::ShapePipeline&, gpu::Batch& batch) {
                // Set the defaults needed for a simple program
                batch.setResourceTexture(render::ShapePipeline::Slot::MAP::ALBEDO,
                    DependencyManager::get<TextureCache>()->getWhiteTexture());
                batch.setResourceTexture(render::ShapePipeline::Slot::MAP::NORMAL_FITTING,
                    DependencyManager::get<TextureCache>()->getNormalFittingTexture());
            }
        );
    GeometryCache::_simpleWirePipeline =
        std::make_shared<render::ShapePipeline>(getSimplePipeline(false, false, true, true), nullptr,
            [](const render::ShapePipeline&, gpu::Batch& batch) {});
}

GeometryCache::~GeometryCache() {
#ifdef WANT_DEBUG
    qCDebug(renderutils) << "GeometryCache::~GeometryCache()... ";
    qCDebug(renderutils) << "    _registeredLine3DVBOs.size():" << _registeredLine3DVBOs.size();
    qCDebug(renderutils) << "    _line3DVBOs.size():" << _line3DVBOs.size();
    qCDebug(renderutils) << "    BatchItemDetails... population:" << GeometryCache::BatchItemDetails::population;
#endif //def WANT_DEBUG
}

void GeometryCache::releaseID(int id) {
    _registeredQuad3DTextures.remove(id);
    _lastRegisteredQuad2DTexture.remove(id);
    _registeredQuad2DTextures.remove(id);
    _lastRegisteredQuad3D.remove(id);
    _registeredQuad3D.remove(id);

    _lastRegisteredQuad2D.remove(id);
    _registeredQuad2D.remove(id);

    _lastRegisteredBevelRects.remove(id);
    _registeredBevelRects.remove(id);

    _lastRegisteredLine3D.remove(id);
    _registeredLine3DVBOs.remove(id);

    _lastRegisteredLine2D.remove(id);
    _registeredLine2DVBOs.remove(id);

    _registeredVertices.remove(id);

    _lastRegisteredDashedLines.remove(id);
    _registeredDashedLines.remove(id);

    _lastRegisteredGridBuffer.remove(id);
    _registeredGridBuffers.remove(id);
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
    if (registered && (!_registeredGridBuffers.contains(id) || _lastRegisteredGridBuffer[id] != key)) {
        GridSchema gridSchema;
        GridBuffer gridBuffer = std::make_shared<gpu::Buffer>(sizeof(GridSchema), (const gpu::Byte*) &gridSchema);

        if (registered && _registeredGridBuffers.contains(id)) {
            gridBuffer = _registeredGridBuffers[id];
        }

        _registeredGridBuffers[id] = gridBuffer;
        _lastRegisteredGridBuffer[id] = key;

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
    useGridPipeline(batch, _registeredGridBuffers[id], isLayered);

    renderQuad(batch, minCorner, maxCorner, MIN_TEX_COORD, MAX_TEX_COORD, color, id);
}

void GeometryCache::updateVertices(int id, const QVector<glm::vec2>& points, const QVector<glm::vec4>& colors) {
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

    float* vertexData = new float[details.vertices * FLOATS_PER_VERTEX];
    float* vertex = vertexData;

    int* colorData = new int[details.vertices];
    int* colorDataAt = colorData;

    const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
    auto pointCount = points.size();
    auto colorCount = colors.size();
    int compactColor = 0;
    for (auto i = 0; i < pointCount; ++i) {
        const auto& point = points[i];
        *(vertex++) = point.x;
        *(vertex++) = point.y;
        *(vertex++) = NORMAL.x;
        *(vertex++) = NORMAL.y;
        *(vertex++) = NORMAL.z;
        if (i < colorCount) {
            const auto& color = colors[i];
            compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                ((int(color.y * 255.0f) & 0xFF) << 8) |
                ((int(color.z * 255.0f) & 0xFF) << 16) |
                ((int(color.w * 255.0f) & 0xFF) << 24);
        }
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

void GeometryCache::updateVertices(int id, const QVector<glm::vec2>& points, const glm::vec4& color) {
    updateVertices(id, points, QVector<glm::vec4>({ color }));
}

void GeometryCache::updateVertices(int id, const QVector<glm::vec3>& points, const QVector<glm::vec4>& colors) {
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

    // Default to white
    int compactColor = 0xFFFFFFFF;
    float* vertexData = new float[details.vertices * FLOATS_PER_VERTEX];
    float* vertex = vertexData;

    int* colorData = new int[details.vertices];
    int* colorDataAt = colorData;

    const glm::vec3 NORMAL(0.0f, 0.0f, 1.0f);
    auto pointCount = points.size();
    auto colorCount = colors.size();
    for (auto i = 0; i < pointCount; ++i) {
        const glm::vec3& point = points[i];
        if (i < colorCount) {
            const glm::vec4& color = colors[i];
            compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                ((int(color.y * 255.0f) & 0xFF) << 8) |
                ((int(color.z * 255.0f) & 0xFF) << 16) |
                ((int(color.w * 255.0f) & 0xFF) << 24);
        }
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

void GeometryCache::updateVertices(int id, const QVector<glm::vec3>& points, const glm::vec4& color) {
    updateVertices(id, points, QVector<glm::vec4>({ color }));
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
    BatchItemDetails& details = _registeredBevelRects[id];
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
    BatchItemDetails& details = _registeredQuad2D[id];

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

    Vec4PairVec4 key(Vec4Pair(glm::vec4(minCorner.x, minCorner.y, maxCorner.x, maxCorner.y),
        glm::vec4(texCoordMinCorner.x, texCoordMinCorner.y, texCoordMaxCorner.x, texCoordMaxCorner.y)),
        color);
    BatchItemDetails& details = _registeredQuad2DTextures[id];

    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (details.isCreated) {
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
    BatchItemDetails& details = _registeredQuad3D[id];

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
        Vec4Pair(glm::vec4(texCoordTopLeft.x, texCoordTopLeft.y, texCoordBottomRight.x, texCoordBottomRight.y),
        color));

    BatchItemDetails& details = _registeredQuad3DTextures[id];

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
    BatchItemDetails& details = _registeredDashedLines[id];

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
isCreated(false) {
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
isCreated(other.isCreated) {
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
    uniformBuffer.reset();
    verticesBuffer.reset();
    colorBuffer.reset();
    streamFormat.reset();
    stream.reset();
}

void GeometryCache::renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2,
    const glm::vec4& color1, const glm::vec4& color2, int id) {

    bool registered = (id != UNKNOWN_ID);
    Vec3Pair key(p1, p2);

    BatchItemDetails& details = _registeredLine3DVBOs[id];

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
            p2.x, p2.y, p2.z, NORMAL.x, NORMAL.y, NORMAL.z };

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

    BatchItemDetails& details = _registeredLine2DVBOs[id];

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


void GeometryCache::renderGlowLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2,
    const glm::vec4& color, float glowIntensity, float glowWidth, int id) {

    // Disable glow lines on OSX
#ifndef Q_OS_WIN
    glowIntensity = 0.0f;
#endif

    if (glowIntensity <= 0) {
        bindSimpleProgram(batch, false, false, false, true, false);
        renderLine(batch, p1, p2, color, id);
        return;
    }

    // Compile the shaders
    static const uint32_t LINE_DATA_SLOT = 1;
    static std::once_flag once;
    std::call_once(once, [&] {
        auto state = std::make_shared<gpu::State>();
        auto VS = gpu::Shader::createVertex(std::string(glowLine_vert));
        auto PS = gpu::Shader::createPixel(std::string(glowLine_frag));
        auto program = gpu::Shader::createProgram(VS, PS);
        state->setCullMode(gpu::State::CULL_NONE);
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setBlendFunction(true, 
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("lineData"), LINE_DATA_SLOT));
        gpu::Shader::makeProgram(*program, slotBindings);
        _glowLinePipeline = gpu::Pipeline::create(program, state);
    });

    batch.setPipeline(_glowLinePipeline);

    Vec3Pair key(p1, p2);
    bool registered = (id != UNKNOWN_ID);
    BatchItemDetails& details = _registeredLine3DVBOs[id];

    // if this is a registered quad, and we have buffers, then check to see if the geometry changed and rebuild if needed
    if (registered && details.isCreated) {
        Vec3Pair& lastKey = _lastRegisteredLine3D[id];
        if (lastKey != key) {
            details.clear();
            _lastRegisteredLine3D[id] = key;
        }
    }

    const int NUM_VERTICES = 4;
    if (!details.isCreated) {
        details.isCreated = true;
        details.uniformBuffer = std::make_shared<gpu::Buffer>();

        struct LineData {
            vec4 p1;
            vec4 p2;
            vec4 color;
        };

        LineData lineData { vec4(p1, 1.0f), vec4(p2, 1.0f), color };
        details.uniformBuffer->resize(sizeof(LineData));
        details.uniformBuffer->setSubData(0, lineData);
    }

    // The shader requires no vertices, only uniforms.
    batch.setUniformBuffer(LINE_DATA_SLOT, details.uniformBuffer);
    batch.draw(gpu::TRIANGLE_STRIP, NUM_VERTICES, 0);
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
        IS_TRANSPARENT_FLAG,
        IS_CULLED_FLAG,
        IS_UNLIT_FLAG,
        HAS_DEPTH_BIAS_FLAG,

        NUM_FLAGS,
    };

    enum Flag {
        IS_TEXTURED = (1 << IS_TEXTURED_FLAG),
        IS_TRANSPARENT = (1 << IS_TRANSPARENT_FLAG),
        IS_CULLED = (1 << IS_CULLED_FLAG),
        IS_UNLIT = (1 << IS_UNLIT_FLAG),
        HAS_DEPTH_BIAS = (1 << HAS_DEPTH_BIAS_FLAG),
    };
    typedef unsigned short Flags;

    bool isFlag(short flagNum) const { return bool((_flags & flagNum) != 0); }

    bool isTextured() const { return isFlag(IS_TEXTURED); }
    bool isTransparent() const { return isFlag(IS_TRANSPARENT); }
    bool isCulled() const { return isFlag(IS_CULLED); }
    bool isUnlit() const { return isFlag(IS_UNLIT); }
    bool hasDepthBias() const { return isFlag(HAS_DEPTH_BIAS); }

    Flags _flags = 0;
    short _spare = 0;

    int getRaw() const { return *reinterpret_cast<const int*>(this); }


    SimpleProgramKey(bool textured = false, bool transparent = false, bool culled = true,
        bool unlit = false, bool depthBias = false) {
        _flags = (textured ? IS_TEXTURED : 0) | (transparent ? IS_TRANSPARENT : 0) | (culled ? IS_CULLED : 0) |
            (unlit ? IS_UNLIT : 0) | (depthBias ? HAS_DEPTH_BIAS : 0);
    }

    SimpleProgramKey(int bitmask) : _flags(bitmask) {}
};

inline uint qHash(const SimpleProgramKey& key, uint seed) {
    return qHash(key.getRaw(), seed);
}

inline bool operator==(const SimpleProgramKey& a, const SimpleProgramKey& b) {
    return a.getRaw() == b.getRaw();
}

void GeometryCache::bindOpaqueWebBrowserProgram(gpu::Batch& batch) {
    batch.setPipeline(getOpaqueWebBrowserProgram());
    // Set a default normal map
    batch.setResourceTexture(render::ShapePipeline::Slot::MAP::NORMAL_FITTING,
                             DependencyManager::get<TextureCache>()->getNormalFittingTexture());
}

gpu::PipelinePointer GeometryCache::getOpaqueWebBrowserProgram() {
    static std::once_flag once;
    std::call_once(once, [&]() {
        auto VS = gpu::Shader::createVertex(std::string(simple_vert));
        auto PS = gpu::Shader::createPixel(std::string(simple_opaque_web_browser_frag));

        _simpleOpaqueWebBrowserShader = gpu::Shader::createProgram(VS, PS);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), render::ShapePipeline::Slot::MAP::NORMAL_FITTING));
        gpu::Shader::makeProgram(*_simpleOpaqueWebBrowserShader, slotBindings);
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_NONE);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setBlendFunction(false,
                                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        _simpleOpaqueWebBrowserPipeline = gpu::Pipeline::create(_simpleOpaqueWebBrowserShader, state);
    });

    return _simpleOpaqueWebBrowserPipeline;
}

void GeometryCache::bindTransparentWebBrowserProgram(gpu::Batch& batch) {
    batch.setPipeline(getTransparentWebBrowserProgram());
    // Set a default normal map
    batch.setResourceTexture(render::ShapePipeline::Slot::MAP::NORMAL_FITTING,
                             DependencyManager::get<TextureCache>()->getNormalFittingTexture());
}

gpu::PipelinePointer GeometryCache::getTransparentWebBrowserProgram() {
    static std::once_flag once;
    std::call_once(once, [&]() {
        auto VS = gpu::Shader::createVertex(std::string(simple_vert));
        auto PS = gpu::Shader::createPixel(std::string(simple_transparent_web_browser_frag));

        _simpleTransparentWebBrowserShader = gpu::Shader::createProgram(VS, PS);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), render::ShapePipeline::Slot::MAP::NORMAL_FITTING));
        gpu::Shader::makeProgram(*_simpleTransparentWebBrowserShader, slotBindings);
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_NONE);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setBlendFunction(true,
                                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        _simpleTransparentWebBrowserPipeline = gpu::Pipeline::create(_simpleTransparentWebBrowserShader, state);
    });

    return _simpleTransparentWebBrowserPipeline;
}

void GeometryCache::bindSimpleProgram(gpu::Batch& batch, bool textured, bool transparent, bool culled, bool unlit, bool depthBiased) {
    batch.setPipeline(getSimplePipeline(textured, transparent, culled, unlit, depthBiased));

    // If not textured, set a default albedo map
    if (!textured) {
        batch.setResourceTexture(render::ShapePipeline::Slot::MAP::ALBEDO,
            DependencyManager::get<TextureCache>()->getWhiteTexture());
    }
    // Set a default normal map
    batch.setResourceTexture(render::ShapePipeline::Slot::MAP::NORMAL_FITTING,
        DependencyManager::get<TextureCache>()->getNormalFittingTexture());
}

gpu::PipelinePointer GeometryCache::getSimplePipeline(bool textured, bool transparent, bool culled, bool unlit, bool depthBiased) {
    SimpleProgramKey config { textured, transparent, culled, unlit, depthBiased };

    // Compile the shaders
    static std::once_flag once;
    std::call_once(once, [&]() {
        auto VS = gpu::Shader::createVertex(std::string(simple_vert));
        auto PS = gpu::Shader::createPixel(std::string(simple_textured_frag));
        auto PSUnlit = gpu::Shader::createPixel(std::string(simple_textured_unlit_frag));

        _simpleShader = gpu::Shader::createProgram(VS, PS);
        _unlitShader = gpu::Shader::createProgram(VS, PSUnlit);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), render::ShapePipeline::Slot::MAP::NORMAL_FITTING));
        gpu::Shader::makeProgram(*_simpleShader, slotBindings);
        gpu::Shader::makeProgram(*_unlitShader, slotBindings);
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
    state->setBlendFunction(config.isTransparent(),
        gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    gpu::ShaderPointer program = (config.isUnlit()) ? _unlitShader : _simpleShader;
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

void renderInstances(gpu::Batch& batch, const glm::vec4& color, bool isWire,
    const render::ShapePipelinePointer& pipeline, GeometryCache::Shape shape) {
    // Add pipeline to name
    std::string instanceName = (isWire ? "wire_shapes_" : "solid_shapes_") + std::to_string(shape) + "_" + std::to_string(std::hash<render::ShapePipelinePointer>()(pipeline));

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

void GeometryCache::renderSolidShapeInstance(gpu::Batch& batch, GeometryCache::Shape shape, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    renderInstances(batch, color, false, pipeline, shape);
}

void GeometryCache::renderWireShapeInstance(gpu::Batch& batch, GeometryCache::Shape shape, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    renderInstances(batch, color, true, pipeline, shape);
}


void GeometryCache::renderSolidSphereInstance(gpu::Batch& batch, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    renderInstances(batch, color, false, pipeline, GeometryCache::Sphere);
}

void GeometryCache::renderWireSphereInstance(gpu::Batch& batch, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    renderInstances(batch, color, true, pipeline, GeometryCache::Sphere);
}

// Enable this in a debug build to cause 'box' entities to iterate through all the
// available shape types, both solid and wireframes
//#define DEBUG_SHAPES

void GeometryCache::renderSolidCubeInstance(gpu::Batch& batch, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
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
    renderInstances(batch, color, false, pipeline, GeometryCache::Cube);
#endif
}

void GeometryCache::renderWireCubeInstance(gpu::Batch& batch, const glm::vec4& color, const render::ShapePipelinePointer& pipeline) {
    static const std::string INSTANCE_NAME = __FUNCTION__;
    renderInstances(batch, color, true, pipeline, GeometryCache::Cube);
}
