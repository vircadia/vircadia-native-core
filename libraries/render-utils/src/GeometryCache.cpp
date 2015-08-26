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

//#define WANT_DEBUG

const int GeometryCache::UNKNOWN_ID = -1;

GeometryCache::GeometryCache() :
    _nextID(0)
{
    const qint64 GEOMETRY_DEFAULT_UNUSED_MAX_SIZE = DEFAULT_UNUSED_MAX_SIZE;
    setUnusedResourceCacheSize(GEOMETRY_DEFAULT_UNUSED_MAX_SIZE);
}

GeometryCache::~GeometryCache() {
    #ifdef WANT_DEBUG
        qCDebug(renderutils) << "GeometryCache::~GeometryCache()... ";
        qCDebug(renderutils) << "    _registeredLine3DVBOs.size():" << _registeredLine3DVBOs.size();
        qCDebug(renderutils) << "    _line3DVBOs.size():" << _line3DVBOs.size();
        qCDebug(renderutils) << "    BatchItemDetails... population:" << GeometryCache::BatchItemDetails::population;
    #endif //def WANT_DEBUG
}

QSharedPointer<Resource> GeometryCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
                                                        bool delayLoad, const void* extra) {
    // NetworkGeometry is no longer a subclass of Resource, but requires this method because, it is pure virtual.
    assert(false);
    return QSharedPointer<Resource>();
}

const int NUM_VERTICES_PER_TRIANGLE = 3;
const int NUM_TRIANGLES_PER_QUAD = 2;
const int NUM_VERTICES_PER_TRIANGULATED_QUAD = NUM_VERTICES_PER_TRIANGLE * NUM_TRIANGLES_PER_QUAD;
const int NUM_COORDS_PER_VERTEX = 3;

void GeometryCache::renderSphere(gpu::Batch& batch, float radius, int slices, int stacks, const glm::vec4& color, bool solid, int id) {
    bool registered = (id != UNKNOWN_ID);

    Vec2Pair radiusKey(glm::vec2(radius, slices), glm::vec2(stacks, 0));
    IntPair slicesStacksKey(slices, stacks);
    Vec3Pair colorKey(glm::vec3(color.x, color.y, slices), glm::vec3(color.z, color.w, stacks));

    int vertices = slices * (stacks - 1) + 2;    
    int indices = slices * (stacks - 1) * NUM_VERTICES_PER_TRIANGULATED_QUAD;

    if ((registered && (!_registeredSphereVertices.contains(id) || _lastRegisteredSphereVertices[id] != radiusKey))
        || (!registered && !_sphereVertices.contains(radiusKey))) {

        if (registered && _registeredSphereVertices.contains(id)) {
            _registeredSphereVertices[id].reset();
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderSphere()... RELEASING REGISTERED VERTICES BUFFER";
            #endif
        }

        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        if (registered) {
            _registeredSphereVertices[id] = verticesBuffer;
            _lastRegisteredSphereVertices[id] = radiusKey;
        } else {
            _sphereVertices[radiusKey] = verticesBuffer;
        }

        GLfloat* vertexData = new GLfloat[vertices * NUM_COORDS_PER_VERTEX];
        GLfloat* vertex = vertexData;

        // south pole
        *(vertex++) = 0.0f;
        *(vertex++) = 0.0f;
        *(vertex++) = -1.0f * radius;

        //add stacks vertices climbing up Y axis
        for (int i = 1; i < stacks; i++) {
            float phi = PI * (float)i / (float)(stacks) - PI_OVER_TWO;
            float z = sinf(phi) * radius;
            float stackRadius = cosf(phi) * radius;
            
            for (int j = 0; j < slices; j++) {
                float theta = TWO_PI * (float)j / (float)slices;

                *(vertex++) = sinf(theta) * stackRadius;
                *(vertex++) = cosf(theta) * stackRadius;
                *(vertex++) = z;
            }
        }

        // north pole
        *(vertex++) = 0.0f;
        *(vertex++) = 0.0f;
        *(vertex++) = 1.0f * radius;

        verticesBuffer->append(sizeof(GLfloat) * vertices * NUM_COORDS_PER_VERTEX, (gpu::Byte*) vertexData);
        delete[] vertexData;

        #ifdef WANT_DEBUG
            qCDebug(renderutils) << "GeometryCache::renderSphere()... --- CREATING VERTICES BUFFER";
            qCDebug(renderutils) << "    radius:" << radius;
            qCDebug(renderutils) << "    slices:" << slices;
            qCDebug(renderutils) << "    stacks:" << stacks;

            qCDebug(renderutils) << "    _sphereVertices.size():" << _sphereVertices.size();
        #endif
    }
    #ifdef WANT_DEBUG
    else if (registered) {
        qCDebug(renderutils) << "renderSphere()... REUSING PREVIOUSLY REGISTERED VERTICES BUFFER";
    }
    #endif

    if ((registered && (!_registeredSphereIndices.contains(id) || _lastRegisteredSphereIndices[id] != slicesStacksKey))
        || (!registered && !_sphereIndices.contains(slicesStacksKey))) {

        if (registered && _registeredSphereIndices.contains(id)) {
            _registeredSphereIndices[id].reset();
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderSphere()... RELEASING REGISTERED INDICES BUFFER";
            #endif
        }

        auto indicesBuffer = std::make_shared<gpu::Buffer>();
        if (registered) {
            _registeredSphereIndices[id] = indicesBuffer;
            _lastRegisteredSphereIndices[id] = slicesStacksKey;
        } else {
            _sphereIndices[slicesStacksKey] = indicesBuffer;
        }

        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        
        int indexCount = 0;

        // South cap
        GLushort bottom = 0;
        GLushort top = 1;
        for (int i = 0; i < slices; i++) {    
            *(index++) = bottom;
            *(index++) = top + i;
            *(index++) = top + (i + 1) % slices;
            
            indexCount += 3;
        }

        // (stacks - 2) ribbons
        for (int i = 0; i < stacks - 2; i++) {
            bottom = i * slices + 1;
            top = bottom + slices;
            for (int j = 0; j < slices; j++) {
                int next = (j + 1) % slices;
                
                *(index++) = top + next;
                *(index++) = bottom + j;
                *(index++) = top + j;

                indexCount += 3;
                
                *(index++) = bottom + next;
                *(index++) = bottom + j;
                *(index++) = top + next;

                indexCount += 3;

            }
        }
        
        // north cap
        bottom = (stacks - 2) * slices + 1;
        top = bottom + slices;
        for (int i = 0; i < slices; i++) {    
            *(index++) = bottom + (i + 1) % slices;
            *(index++) = bottom + i;
            *(index++) = top;
    
            indexCount += 3;

        }
        indicesBuffer->append(sizeof(GLushort) * indices, (gpu::Byte*) indexData);
        delete[] indexData;
        
        #ifdef WANT_DEBUG
            qCDebug(renderutils) << "GeometryCache::renderSphere()... --- CREATING INDICES BUFFER";
            qCDebug(renderutils) << "    radius:" << radius;
            qCDebug(renderutils) << "    slices:" << slices;
            qCDebug(renderutils) << "    stacks:" << stacks;
            qCDebug(renderutils) << "indexCount:" << indexCount;
            qCDebug(renderutils) << "   indices:" << indices;
            qCDebug(renderutils) << "    _sphereIndices.size():" << _sphereIndices.size();
        #endif
    }
    #ifdef WANT_DEBUG
    else if (registered) {
        qCDebug(renderutils) << "renderSphere()... REUSING PREVIOUSLY REGISTERED INDICES BUFFER";
    }
    #endif

    if ((registered && (!_registeredSphereColors.contains(id) || _lastRegisteredSphereColors[id] != colorKey)) 
        || (!registered && !_sphereColors.contains(colorKey))) {

        if (registered && _registeredSphereColors.contains(id)) {
            _registeredSphereColors[id].reset();
            #ifdef WANT_DEBUG
                qCDebug(renderutils) << "renderSphere()... RELEASING REGISTERED COLORS BUFFER";
            #endif
        }
        
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        if (registered) {
            _registeredSphereColors[id] = colorBuffer;
            _lastRegisteredSphereColors[id] = colorKey;
        } else {
            _sphereColors[colorKey] = colorBuffer;
        }

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

        #ifdef WANT_DEBUG
            qCDebug(renderutils) << "GeometryCache::renderSphere()... --- CREATING COLORS BUFFER";
            qCDebug(renderutils) << "    vertices:" << vertices;
            qCDebug(renderutils) << "    color:" << color;
            qCDebug(renderutils) << "    slices:" << slices;
            qCDebug(renderutils) << "    stacks:" << stacks;
            qCDebug(renderutils) << "    _sphereColors.size():" << _sphereColors.size();
        #endif
    }
    #ifdef WANT_DEBUG
    else if (registered) {
        qCDebug(renderutils) << "renderSphere()... REUSING PREVIOUSLY REGISTERED COLORS BUFFER";
    }
    #endif

    gpu::BufferPointer verticesBuffer = registered ? _registeredSphereVertices[id] : _sphereVertices[radiusKey];
    gpu::BufferPointer indicesBuffer = registered ? _registeredSphereIndices[id] : _sphereIndices[slicesStacksKey];
    gpu::BufferPointer colorBuffer = registered ? _registeredSphereColors[id] : _sphereColors[colorKey];
    
    const int VERTICES_SLOT = 0;
    const int NORMALS_SLOT = 1;
    const int COLOR_SLOT = 2;
    static gpu::Stream::FormatPointer streamFormat;
    static gpu::Element positionElement, normalElement, colorElement;
    if (!streamFormat) {
        streamFormat = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
        streamFormat->setAttribute(gpu::Stream::POSITION, VERTICES_SLOT, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        streamFormat->setAttribute(gpu::Stream::NORMAL, NORMALS_SLOT, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
        streamFormat->setAttribute(gpu::Stream::COLOR, COLOR_SLOT, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));
        positionElement = streamFormat->getAttributes().at(gpu::Stream::POSITION)._element;
        normalElement = streamFormat->getAttributes().at(gpu::Stream::NORMAL)._element;
        colorElement = streamFormat->getAttributes().at(gpu::Stream::COLOR)._element;
    }

    gpu::BufferView verticesView(verticesBuffer, positionElement);
    gpu::BufferView normalsView(verticesBuffer, normalElement);
    gpu::BufferView colorView(colorBuffer, colorElement);
    
    batch.setInputFormat(streamFormat);
    batch.setInputBuffer(VERTICES_SLOT, verticesView);
    batch.setInputBuffer(NORMALS_SLOT, normalsView);
    batch.setInputBuffer(COLOR_SLOT, colorView);
    batch.setIndexBuffer(gpu::UINT16, indicesBuffer, 0);

    if (solid) {
        batch.drawIndexed(gpu::TRIANGLES, indices);
    } else {
        batch.drawIndexed(gpu::LINES, indices);
    }
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

void GeometryCache::renderSolidCube(gpu::Batch& batch, float size, const glm::vec4& color) {
    Vec2Pair colorKey(glm::vec2(color.x, color.y), glm::vec2(color.z, color.y));
    const int FLOATS_PER_VERTEX = 3;
    const int VERTICES_PER_FACE = 4;
    const int NUMBER_OF_FACES = 6;
    const int TRIANGLES_PER_FACE = 2;
    const int VERTICES_PER_TRIANGLE = 3;
    const int vertices = NUMBER_OF_FACES * VERTICES_PER_FACE;
    const int indices = NUMBER_OF_FACES * TRIANGLES_PER_FACE * VERTICES_PER_TRIANGLE;
    const int vertexPoints = vertices * FLOATS_PER_VERTEX;
    const int VERTEX_STRIDE = sizeof(GLfloat) * FLOATS_PER_VERTEX * 2; // vertices and normals
    const int NORMALS_OFFSET = sizeof(GLfloat) * FLOATS_PER_VERTEX;

    if (!_solidCubeVertices.contains(size)) {
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        _solidCubeVertices[size] = verticesBuffer;

        GLfloat* vertexData = new GLfloat[vertexPoints * 2]; // vertices and normals
        GLfloat* vertex = vertexData;
        float halfSize = size / 2.0f;

        static GLfloat cannonicalVertices[vertexPoints] = 
                                    { 1, 1, 1,  -1, 1, 1,  -1,-1, 1,   1,-1, 1,   // v0,v1,v2,v3 (front)
                                      1, 1, 1,   1,-1, 1,   1,-1,-1,   1, 1,-1,   // v0,v3,v4,v5 (right)
                                      1, 1, 1,   1, 1,-1,  -1, 1,-1,  -1, 1, 1,   // v0,v5,v6,v1 (top)
                                     -1, 1, 1,  -1, 1,-1,  -1,-1,-1,  -1,-1, 1,   // v1,v6,v7,v2 (left)
                                     -1,-1,-1,   1,-1,-1,   1,-1, 1,  -1,-1, 1,   // v7,v4,v3,v2 (bottom)
                                      1,-1,-1,  -1,-1,-1,  -1, 1,-1,   1, 1,-1 }; // v4,v7,v6,v5 (back)

        // normal array
        static GLfloat cannonicalNormals[vertexPoints]  = 
                                  { 0, 0, 1,   0, 0, 1,   0, 0, 1,   0, 0, 1,   // v0,v1,v2,v3 (front)
                                    1, 0, 0,   1, 0, 0,   1, 0, 0,   1, 0, 0,   // v0,v3,v4,v5 (right)
                                    0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0,   // v0,v5,v6,v1 (top)
                                   -1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0,   // v1,v6,v7,v2 (left)
                                    0,-1, 0,   0,-1, 0,   0,-1, 0,   0,-1, 0,   // v7,v4,v3,v2 (bottom)
                                    0, 0,-1,   0, 0,-1,   0, 0,-1,   0, 0,-1 }; // v4,v7,v6,v5 (back)


        GLfloat* cannonicalVertex = &cannonicalVertices[0];
        GLfloat* cannonicalNormal = &cannonicalNormals[0];

        for (int i = 0; i < vertices; i++) {
            // vertices
            *(vertex++) = halfSize * *cannonicalVertex++;
            *(vertex++) = halfSize * *cannonicalVertex++;
            *(vertex++) = halfSize * *cannonicalVertex++;

            //normals
            *(vertex++) = *cannonicalNormal++;
            *(vertex++) = *cannonicalNormal++;
            *(vertex++) = *cannonicalNormal++;
        }

        verticesBuffer->append(sizeof(GLfloat) * vertexPoints * 2, (gpu::Byte*) vertexData);
    }

    if (!_solidCubeIndexBuffer) {
        static GLubyte cannonicalIndices[indices]  = 
                                    { 0, 1, 2,   2, 3, 0,      // front
                                      4, 5, 6,   6, 7, 4,      // right
                                      8, 9,10,  10,11, 8,      // top
                                     12,13,14,  14,15,12,      // left
                                     16,17,18,  18,19,16,      // bottom
                                     20,21,22,  22,23,20 };    // back
        
        auto indexBuffer = std::make_shared<gpu::Buffer>();
        _solidCubeIndexBuffer = indexBuffer;
    
        _solidCubeIndexBuffer->append(sizeof(cannonicalIndices), (gpu::Byte*) cannonicalIndices);
    }

    if (!_solidCubeColors.contains(colorKey)) {
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        _solidCubeColors[colorKey] = colorBuffer;

        const int NUM_COLOR_SCALARS_PER_CUBE = 24;
        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);
        int colors[NUM_COLOR_SCALARS_PER_CUBE] = { compactColor, compactColor, compactColor, compactColor,
                                                   compactColor, compactColor, compactColor, compactColor,
                                                   compactColor, compactColor, compactColor, compactColor,
                                                   compactColor, compactColor, compactColor, compactColor,
                                                   compactColor, compactColor, compactColor, compactColor,
                                                   compactColor, compactColor, compactColor, compactColor };

        colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);
    }
    gpu::BufferPointer verticesBuffer = _solidCubeVertices[size];
    gpu::BufferPointer colorBuffer = _solidCubeColors[colorKey];

    const int VERTICES_SLOT = 0;
    const int NORMALS_SLOT = 1;
    const int COLOR_SLOT = 2;
    static gpu::Stream::FormatPointer streamFormat;
    static gpu::Element positionElement, normalElement, colorElement;
    if (!streamFormat) {
        streamFormat = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
        streamFormat->setAttribute(gpu::Stream::POSITION, VERTICES_SLOT, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        streamFormat->setAttribute(gpu::Stream::NORMAL, NORMALS_SLOT, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
        streamFormat->setAttribute(gpu::Stream::COLOR, COLOR_SLOT, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));
        positionElement = streamFormat->getAttributes().at(gpu::Stream::POSITION)._element;
        normalElement = streamFormat->getAttributes().at(gpu::Stream::NORMAL)._element;
        colorElement = streamFormat->getAttributes().at(gpu::Stream::COLOR)._element;
    }
    
    
    gpu::BufferView verticesView(verticesBuffer, 0, verticesBuffer->getSize(), VERTEX_STRIDE, positionElement);
    gpu::BufferView normalsView(verticesBuffer, NORMALS_OFFSET, verticesBuffer->getSize(), VERTEX_STRIDE, normalElement);
    gpu::BufferView colorView(colorBuffer, streamFormat->getAttributes().at(gpu::Stream::COLOR)._element);
    
    batch.setInputFormat(streamFormat);
    batch.setInputBuffer(VERTICES_SLOT, verticesView);
    batch.setInputBuffer(NORMALS_SLOT, normalsView);
    batch.setInputBuffer(COLOR_SLOT, colorView);
    batch.setIndexBuffer(gpu::UINT8, _solidCubeIndexBuffer, 0);
    batch.drawIndexed(gpu::TRIANGLES, indices);
}

void GeometryCache::renderWireCube(gpu::Batch& batch, float size, const glm::vec4& color) {
    Vec2Pair colorKey(glm::vec2(color.x, color.y),glm::vec2(color.z, color.y));
    const int FLOATS_PER_VERTEX = 3;
    const int VERTICES_PER_EDGE = 2;
    const int TOP_EDGES = 4;
    const int BOTTOM_EDGES = 4;
    const int SIDE_EDGES = 4;
    const int vertices = 8;
    const int indices = (TOP_EDGES + BOTTOM_EDGES + SIDE_EDGES) * VERTICES_PER_EDGE;

    if (!_cubeVerticies.contains(size)) {
        auto verticesBuffer = std::make_shared<gpu::Buffer>();
        _cubeVerticies[size] = verticesBuffer;

        int vertexPoints = vertices * FLOATS_PER_VERTEX;
        GLfloat* vertexData = new GLfloat[vertexPoints]; // only vertices, no normals because we're a wire cube
        GLfloat* vertex = vertexData;
        float halfSize = size / 2.0f;
        
        static GLfloat cannonicalVertices[] = 
                                    { 1, 1, 1,   1, 1,-1,  -1, 1,-1,  -1, 1, 1,   // v0, v1, v2, v3 (top)
                                      1,-1, 1,   1,-1,-1,  -1,-1,-1,  -1,-1, 1    // v4, v5, v6, v7 (bottom)
                                    };

        for (int i = 0; i < vertexPoints; i++) {
            vertex[i] = cannonicalVertices[i] * halfSize;
        }

        verticesBuffer->append(sizeof(GLfloat) * vertexPoints, (gpu::Byte*) vertexData); // I'm skeptical that this is right
    }

    if (!_wireCubeIndexBuffer) {
        static GLubyte cannonicalIndices[indices]  = { 
                                      0, 1,  1, 2,  2, 3,  3, 0, // (top)
                                      4, 5,  5, 6,  6, 7,  7, 4, // (bottom)
                                      0, 4,  1, 5,  2, 6,  3, 7, // (side edges)
                                    };
        
        auto indexBuffer = std::make_shared<gpu::Buffer>();
        _wireCubeIndexBuffer = indexBuffer;
    
        _wireCubeIndexBuffer->append(sizeof(cannonicalIndices), (gpu::Byte*) cannonicalIndices);
    }

    if (!_cubeColors.contains(colorKey)) {
        auto colorBuffer = std::make_shared<gpu::Buffer>();
        _cubeColors[colorKey] = colorBuffer;

        const int NUM_COLOR_SCALARS_PER_CUBE = 8;
        int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
                            ((int(color.y * 255.0f) & 0xFF) << 8) |
                            ((int(color.z * 255.0f) & 0xFF) << 16) |
                            ((int(color.w * 255.0f) & 0xFF) << 24);
        int colors[NUM_COLOR_SCALARS_PER_CUBE] = { compactColor, compactColor, compactColor, compactColor,
                                                   compactColor, compactColor, compactColor, compactColor };

        colorBuffer->append(sizeof(colors), (gpu::Byte*) colors);
    }
    gpu::BufferPointer verticesBuffer = _cubeVerticies[size];
    gpu::BufferPointer colorBuffer = _cubeColors[colorKey];

    const int VERTICES_SLOT = 0;
    const int COLOR_SLOT = 1;
    static gpu::Stream::FormatPointer streamFormat;
    static gpu::Element positionElement, colorElement;
    if (!streamFormat) {
        streamFormat = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
        streamFormat->setAttribute(gpu::Stream::POSITION, VERTICES_SLOT, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        streamFormat->setAttribute(gpu::Stream::COLOR, COLOR_SLOT, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));
        positionElement = streamFormat->getAttributes().at(gpu::Stream::POSITION)._element;
        colorElement = streamFormat->getAttributes().at(gpu::Stream::COLOR)._element;
    }
   
    gpu::BufferView verticesView(verticesBuffer, positionElement);
    gpu::BufferView colorView(colorBuffer, colorElement);
    
    batch.setInputFormat(streamFormat);
    batch.setInputBuffer(VERTICES_SLOT, verticesView);
    batch.setInputBuffer(COLOR_SLOT, colorView);
    batch.setIndexBuffer(gpu::UINT8, _wireCubeIndexBuffer, 0);
    batch.drawIndexed(gpu::LINES, indices);
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

void GeometryCache::renderUnitCube(gpu::Batch& batch) {
    static const glm::vec4 color(1);
    renderSolidCube(batch, 1, color);
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

GeometryReader::GeometryReader(const QUrl& url, const QByteArray& data, const QVariantHash& mapping) :
    _url(url),
    _data(data),
    _mapping(mapping) {
}

void GeometryReader::run() {
    try {
        if (_data.isEmpty()) {
            throw QString("Reply is NULL ?!");
        }
        QString urlname = _url.path().toLower();
        bool urlValid = true;
        urlValid &= !urlname.isEmpty();
        urlValid &= !_url.path().isEmpty();
        urlValid &= _url.path().toLower().endsWith(".fbx") || _url.path().toLower().endsWith(".obj");

        if (urlValid) {
            // Let's read the binaries from the network
            FBXGeometry* fbxgeo = nullptr;
            if (_url.path().toLower().endsWith(".fbx")) {
                const bool grabLightmaps = true;
                const float lightmapLevel = 1.0f;
                fbxgeo = readFBX(_data, _mapping, _url.path(), grabLightmaps, lightmapLevel);
            } else if (_url.path().toLower().endsWith(".obj")) {
                fbxgeo = OBJReader().readOBJ(_data, _mapping);
            } else {
                QString errorStr("usupported format");
                emit onError(NetworkGeometry::ModelParseError, errorStr);
            }
            emit onSuccess(fbxgeo);
        } else {
            throw QString("url is invalid");
        }

    } catch (const QString& error) {
        qCDebug(renderutils) << "Error reading " << _url << ": " << error;
        emit onError(NetworkGeometry::ModelParseError, error);
    }
}

NetworkGeometry::NetworkGeometry(const QUrl& url, bool delayLoad, const QVariantHash& mapping, const QUrl& textureBaseUrl) :
    _url(url),
    _mapping(mapping),
    _textureBaseUrl(textureBaseUrl.isValid() ? textureBaseUrl : url) {

    if (delayLoad) {
        _state = DelayState;
    } else {
        attemptRequestInternal();
    }
}

NetworkGeometry::~NetworkGeometry() {
    if (_resource) {
        _resource->deleteLater();
    }
}

void NetworkGeometry::attemptRequest() {
    if (_state == DelayState) {
        attemptRequestInternal();
    }
}

void NetworkGeometry::attemptRequestInternal() {
    if (_url.path().toLower().endsWith(".fst")) {
        _mappingUrl = _url;
        requestMapping(_url);
    } else {
        _modelUrl = _url;
        requestModel(_url);
    }
}

bool NetworkGeometry::isLoaded() const {
    return _state == SuccessState;
}

bool NetworkGeometry::isLoadedWithTextures() const {
    if (!isLoaded()) {
        return false;
    }
    if (!_isLoadedWithTextures) {
        for (auto&& mesh : _meshes) {
            for (auto && part : mesh->_parts) {
                if ((part->diffuseTexture && !part->diffuseTexture->isLoaded()) ||
                    (part->normalTexture && !part->normalTexture->isLoaded()) ||
                    (part->specularTexture && !part->specularTexture->isLoaded()) ||
                    (part->emissiveTexture && !part->emissiveTexture->isLoaded())) {
                    return false;
                }
            }
        }
        _isLoadedWithTextures = true;
    }
    return true;
}

void NetworkGeometry::setTextureWithNameToURL(const QString& name, const QUrl& url) {
    if (_meshes.size() > 0) {
        auto textureCache = DependencyManager::get<TextureCache>();
        for (size_t i = 0; i < _meshes.size(); i++) {
            NetworkMesh& mesh = *(_meshes[i].get());
            for (size_t j = 0; j < mesh._parts.size(); j++) {
                NetworkMeshPart& part = *(mesh._parts[j].get());
                QSharedPointer<NetworkTexture> matchingTexture = QSharedPointer<NetworkTexture>();
                if (part.diffuseTextureName == name) {
                    part.diffuseTexture = textureCache->getTexture(url, DEFAULT_TEXTURE, _geometry->meshes[i].isEye);
                } else if (part.normalTextureName == name) {
                    part.normalTexture = textureCache->getTexture(url);
                } else if (part.specularTextureName == name) {
                    part.specularTexture = textureCache->getTexture(url);
                } else if (part.emissiveTextureName == name) {
                    part.emissiveTexture = textureCache->getTexture(url);
                }
            }
        }
    } else {
        qCWarning(renderutils) << "Ignoring setTextureWirthNameToURL() geometry not ready." << name << url;
    }
    _isLoadedWithTextures = false;
}

QStringList NetworkGeometry::getTextureNames() const {
    QStringList result;
    for (size_t i = 0; i < _meshes.size(); i++) {
        const NetworkMesh& mesh = *(_meshes[i].get());
        for (size_t j = 0; j < mesh._parts.size(); j++) {
            const NetworkMeshPart& part = *(mesh._parts[j].get());

            if (!part.diffuseTextureName.isEmpty() && part.diffuseTexture) {
                QString textureURL = part.diffuseTexture->getURL().toString();
                result << part.diffuseTextureName + ":" + textureURL;
            }

            if (!part.normalTextureName.isEmpty() && part.normalTexture) {
                QString textureURL = part.normalTexture->getURL().toString();
                result << part.normalTextureName + ":" + textureURL;
            }

            if (!part.specularTextureName.isEmpty() && part.specularTexture) {
                QString textureURL = part.specularTexture->getURL().toString();
                result << part.specularTextureName + ":" + textureURL;
            }

            if (!part.emissiveTextureName.isEmpty() && part.emissiveTexture) {
                QString textureURL = part.emissiveTexture->getURL().toString();
                result << part.emissiveTextureName + ":" + textureURL;
            }
        }
    }
    return result;
}

void NetworkGeometry::requestMapping(const QUrl& url) {
    _state = RequestMappingState;
    if (_resource) {
        _resource->deleteLater();
    }
    _resource = new Resource(url, false);
    connect(_resource, &Resource::loaded, this, &NetworkGeometry::mappingRequestDone);
    connect(_resource, &Resource::failed, this, &NetworkGeometry::mappingRequestError);
}

void NetworkGeometry::requestModel(const QUrl& url) {
    _state = RequestModelState;
    if (_resource) {
        _resource->deleteLater();
    }
    _modelUrl = url;
    _resource = new Resource(url, false);
    connect(_resource, &Resource::loaded, this, &NetworkGeometry::modelRequestDone);
    connect(_resource, &Resource::failed, this, &NetworkGeometry::modelRequestError);
}

void NetworkGeometry::mappingRequestDone(const QByteArray& data) {
    assert(_state == RequestMappingState);

    // parse the mapping file
    _mapping = FSTReader::readMapping(data);

    QUrl replyUrl = _mappingUrl;
    QString modelUrlStr = _mapping.value("filename").toString();
    if (modelUrlStr.isNull()) {
        qCDebug(renderutils) << "Mapping file " << _url << "has no \"filename\" entry";
        emit onFailure(*this, MissingFilenameInMapping);
    } else {
        // read _textureBase from mapping file, if present
        QString texdir = _mapping.value("texdir").toString();
        if (!texdir.isNull()) {
            if (!texdir.endsWith('/')) {
                texdir += '/';
            }
            _textureBaseUrl = replyUrl.resolved(texdir);
        }

        _modelUrl = replyUrl.resolved(modelUrlStr);
        requestModel(_modelUrl);
    }
}

void NetworkGeometry::mappingRequestError(QNetworkReply::NetworkError error) {
    assert(_state == RequestMappingState);
    _state = ErrorState;
    emit onFailure(*this, MappingRequestError);
}

void NetworkGeometry::modelRequestDone(const QByteArray& data) {
    assert(_state == RequestModelState);

    _state = ParsingModelState;

    // asynchronously parse the model file.
    GeometryReader* geometryReader = new GeometryReader(_modelUrl, data, _mapping);
    connect(geometryReader, SIGNAL(onSuccess(FBXGeometry*)), SLOT(modelParseSuccess(FBXGeometry*)));
    connect(geometryReader, SIGNAL(onError(int, QString)), SLOT(modelParseError(int, QString)));

    QThreadPool::globalInstance()->start(geometryReader);
}

void NetworkGeometry::modelRequestError(QNetworkReply::NetworkError error) {
    assert(_state == RequestModelState);
    _state = ErrorState;
    emit onFailure(*this, ModelRequestError);
}

static NetworkMesh* buildNetworkMesh(const FBXMesh& mesh, const QUrl& textureBaseUrl) {
    auto textureCache = DependencyManager::get<TextureCache>();
    NetworkMesh* networkMesh = new NetworkMesh();

    int totalIndices = 0;
    bool checkForTexcoordLightmap = false;

    // process network parts
    foreach (const FBXMeshPart& part, mesh.parts) {
        NetworkMeshPart* networkPart = new NetworkMeshPart();

        if (!part.diffuseTexture.filename.isEmpty()) {
            networkPart->diffuseTexture = textureCache->getTexture(textureBaseUrl.resolved(QUrl(part.diffuseTexture.filename)), DEFAULT_TEXTURE,
                                                                   mesh.isEye, part.diffuseTexture.content);
            networkPart->diffuseTextureName = part.diffuseTexture.name;
        }
        if (!part.normalTexture.filename.isEmpty()) {
            networkPart->normalTexture = textureCache->getTexture(textureBaseUrl.resolved(QUrl(part.normalTexture.filename)), NORMAL_TEXTURE,
                                                                  false, part.normalTexture.content);
            networkPart->normalTextureName = part.normalTexture.name;
        }
        if (!part.specularTexture.filename.isEmpty()) {
            networkPart->specularTexture = textureCache->getTexture(textureBaseUrl.resolved(QUrl(part.specularTexture.filename)), SPECULAR_TEXTURE,
                                                                    false, part.specularTexture.content);
            networkPart->specularTextureName = part.specularTexture.name;
        }
        if (!part.emissiveTexture.filename.isEmpty()) {
            networkPart->emissiveTexture = textureCache->getTexture(textureBaseUrl.resolved(QUrl(part.emissiveTexture.filename)), EMISSIVE_TEXTURE,
                                                                    false, part.emissiveTexture.content);
            networkPart->emissiveTextureName = part.emissiveTexture.name;
            checkForTexcoordLightmap = true;
        }
        networkMesh->_parts.emplace_back(networkPart);
        totalIndices += (part.quadIndices.size() + part.triangleIndices.size());
    }

    // initialize index buffer
    {
        networkMesh->_indexBuffer = std::make_shared<gpu::Buffer>();
        networkMesh->_indexBuffer->resize(totalIndices * sizeof(int));
        int offset = 0;
        foreach(const FBXMeshPart& part, mesh.parts) {
            networkMesh->_indexBuffer->setSubData(offset, part.quadIndices.size() * sizeof(int),
                                                  (gpu::Byte*) part.quadIndices.constData());
            offset += part.quadIndices.size() * sizeof(int);
            networkMesh->_indexBuffer->setSubData(offset, part.triangleIndices.size() * sizeof(int),
                                                  (gpu::Byte*) part.triangleIndices.constData());
            offset += part.triangleIndices.size() * sizeof(int);
        }
    }

    // initialize vertex buffer
    {
        networkMesh->_vertexBuffer = std::make_shared<gpu::Buffer>();
        // if we don't need to do any blending, the positions/normals can be static
        if (mesh.blendshapes.isEmpty()) {
            int normalsOffset = mesh.vertices.size() * sizeof(glm::vec3);
            int tangentsOffset = normalsOffset + mesh.normals.size() * sizeof(glm::vec3);
            int colorsOffset = tangentsOffset + mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int texCoords1Offset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterIndicesOffset = texCoords1Offset + mesh.texCoords1.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);

            networkMesh->_vertexBuffer->resize(clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4));

            networkMesh->_vertexBuffer->setSubData(0, mesh.vertices.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.vertices.constData());
            networkMesh->_vertexBuffer->setSubData(normalsOffset, mesh.normals.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.normals.constData());
            networkMesh->_vertexBuffer->setSubData(tangentsOffset,
                                                   mesh.tangents.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.tangents.constData());
            networkMesh->_vertexBuffer->setSubData(colorsOffset, mesh.colors.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.colors.constData());
            networkMesh->_vertexBuffer->setSubData(texCoordsOffset,
                                                   mesh.texCoords.size() * sizeof(glm::vec2), (gpu::Byte*) mesh.texCoords.constData());
            networkMesh->_vertexBuffer->setSubData(texCoords1Offset,
                                                   mesh.texCoords1.size() * sizeof(glm::vec2), (gpu::Byte*) mesh.texCoords1.constData());
            networkMesh->_vertexBuffer->setSubData(clusterIndicesOffset,
                                                   mesh.clusterIndices.size() * sizeof(glm::vec4), (gpu::Byte*) mesh.clusterIndices.constData());
            networkMesh->_vertexBuffer->setSubData(clusterWeightsOffset,
                                                   mesh.clusterWeights.size() * sizeof(glm::vec4), (gpu::Byte*) mesh.clusterWeights.constData());

            // otherwise, at least the cluster indices/weights can be static
            networkMesh->_vertexStream = std::make_shared<gpu::BufferStream>();
            networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, 0, sizeof(glm::vec3));
            if (mesh.normals.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, normalsOffset, sizeof(glm::vec3));
            if (mesh.tangents.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, tangentsOffset, sizeof(glm::vec3));
            if (mesh.colors.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, colorsOffset, sizeof(glm::vec3));
            if (mesh.texCoords.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, texCoordsOffset, sizeof(glm::vec2));
            if (mesh.texCoords1.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, texCoords1Offset, sizeof(glm::vec2));
            if (mesh.clusterIndices.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, clusterIndicesOffset, sizeof(glm::vec4));
            if (mesh.clusterWeights.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, clusterWeightsOffset, sizeof(glm::vec4));

            int channelNum = 0;
            networkMesh->_vertexFormat = std::make_shared<gpu::Stream::Format>();
            networkMesh->_vertexFormat->setAttribute(gpu::Stream::POSITION, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
            if (mesh.normals.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::NORMAL, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.tangents.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::TANGENT, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.colors.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::COLOR, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RGB));
            if (mesh.texCoords.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::TEXCOORD, channelNum++, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
            if (mesh.texCoords1.size()) {
                networkMesh->_vertexFormat->setAttribute(gpu::Stream::TEXCOORD1, channelNum++, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
            } else if (checkForTexcoordLightmap && mesh.texCoords.size()) {
                // need lightmap texcoord UV but doesn't have uv#1 so just reuse the same channel
                networkMesh->_vertexFormat->setAttribute(gpu::Stream::TEXCOORD1, channelNum - 1, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
            }
            if (mesh.clusterIndices.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_INDEX, channelNum++, gpu::Element(gpu::VEC4, gpu::NFLOAT, gpu::XYZW));
            if (mesh.clusterWeights.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_WEIGHT, channelNum++, gpu::Element(gpu::VEC4, gpu::NFLOAT, gpu::XYZW));
        }
        else {
            int colorsOffset = mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int clusterIndicesOffset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);

            networkMesh->_vertexBuffer->resize(clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4));
            networkMesh->_vertexBuffer->setSubData(0, mesh.tangents.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.tangents.constData());
            networkMesh->_vertexBuffer->setSubData(colorsOffset, mesh.colors.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.colors.constData());
            networkMesh->_vertexBuffer->setSubData(texCoordsOffset,
                                                   mesh.texCoords.size() * sizeof(glm::vec2), (gpu::Byte*) mesh.texCoords.constData());
            networkMesh->_vertexBuffer->setSubData(clusterIndicesOffset,
                                                   mesh.clusterIndices.size() * sizeof(glm::vec4), (gpu::Byte*) mesh.clusterIndices.constData());
            networkMesh->_vertexBuffer->setSubData(clusterWeightsOffset,
                                                   mesh.clusterWeights.size() * sizeof(glm::vec4), (gpu::Byte*) mesh.clusterWeights.constData());

            networkMesh->_vertexStream = std::make_shared<gpu::BufferStream>();
            if (mesh.tangents.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, 0, sizeof(glm::vec3));
            if (mesh.colors.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, colorsOffset, sizeof(glm::vec3));
            if (mesh.texCoords.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, texCoordsOffset, sizeof(glm::vec2));
            if (mesh.clusterIndices.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, clusterIndicesOffset, sizeof(glm::vec4));
            if (mesh.clusterWeights.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, clusterWeightsOffset, sizeof(glm::vec4));

            int channelNum = 0;
            networkMesh->_vertexFormat = std::make_shared<gpu::Stream::Format>();
            networkMesh->_vertexFormat->setAttribute(gpu::Stream::POSITION, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.normals.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::NORMAL, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.tangents.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::TANGENT, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.colors.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::COLOR, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RGB));
            if (mesh.texCoords.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::TEXCOORD, channelNum++, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
            if (mesh.clusterIndices.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_INDEX, channelNum++, gpu::Element(gpu::VEC4, gpu::NFLOAT, gpu::XYZW));
            if (mesh.clusterWeights.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_WEIGHT, channelNum++, gpu::Element(gpu::VEC4, gpu::NFLOAT, gpu::XYZW));
        }
    }

    return networkMesh;
}

void NetworkGeometry::modelParseSuccess(FBXGeometry* geometry) {
    // assume owner ship of geometry pointer
    _geometry.reset(geometry);

    foreach(const FBXMesh& mesh, _geometry->meshes) {
        _meshes.emplace_back(buildNetworkMesh(mesh, _textureBaseUrl));
    }

    _state = SuccessState;
    emit onSuccess(*this, *_geometry.get());

    delete _resource;
    _resource = nullptr;
}

void NetworkGeometry::modelParseError(int error, QString str) {
    _state = ErrorState;
    emit onFailure(*this, (NetworkGeometry::Error)error);

    delete _resource;
    _resource = nullptr;
}

bool NetworkMeshPart::isTranslucent() const {
    return diffuseTexture && diffuseTexture->isTranslucent();
}

bool NetworkMesh::isPartTranslucent(const FBXMesh& fbxMesh, int partIndex) const {
    assert(partIndex >= 0);
    assert((size_t)partIndex < _parts.size());
    return (_parts.at(partIndex)->isTranslucent() || fbxMesh.parts.at(partIndex).opacity != 1.0f);
}

int NetworkMesh::getTranslucentPartCount(const FBXMesh& fbxMesh) const {
    int count = 0;

    for (size_t i = 0; i < _parts.size(); i++) {
        if (isPartTranslucent(fbxMesh, i)) {
            count++;
        }
    }
    return count;
}
