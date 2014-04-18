//
//  Renderer.cpp
//  interface/src/starfield/renderer
//
//  Created by Chris Barnard on 10/17/13.
//  Based on earlier work by Tobias Schwinger 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "starfield/renderer/Renderer.h"

using namespace starfield;

Renderer::Renderer(InputVertices const& stars, unsigned numStars, unsigned tileResolution) : _dataArray(0l),
_tileArray(0l), _tiling(tileResolution) {
    this->glAlloc();

    Tiling tiling(tileResolution);
    size_t numTiles = tiling.getTileCount();

    // REVISIT: batch arrays are probably oversized, but - hey - they
    // are not very large (unless for insane tiling) and we're better
    // off safe than sorry
    _dataArray = new GpuVertex[numStars];
    _tileArray = new Tile[numTiles + 1];
    _batchOffs = new GLint[numTiles * 2];
    _batchCountArray = new GLsizei[numTiles * 2];

    prepareVertexData(stars, numStars, tiling);

    this->glUpload(numStars);
}

Renderer::~Renderer() {
    delete[] _dataArray;
    delete[] _tileArray;
    delete[] _batchCountArray;
    delete[] _batchOffs;

    this->glFree();
}

void Renderer::render(float perspective, float aspect, mat4 const& orientation, float alpha) {
    float halfPersp = perspective * 0.5f;

    // cancel all translation
    mat4 matrix = orientation;
    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = 0.0f;

    // extract local z vector
    vec3 ahead = vec3(matrix[2]);

    float azimuth = atan2(ahead.x,-ahead.z) + Radians::pi();
    float altitude = atan2(-ahead.y, hypotf(ahead.x, ahead.z));
    angleHorizontalPolar<Radians>(azimuth, altitude);
    float const eps = 0.002f;
    altitude = glm::clamp(altitude, -Radians::halfPi() + eps, Radians::halfPi() - eps);

    matrix = glm::affineInverse(matrix);

    this->_outIndexPos = (unsigned*) _batchOffs;
    this->_wRowVec = -vec3(row(matrix, 2));
    this->_halfPerspectiveAngle = halfPersp;

    TileSelection::Cursor cursor;
    cursor.current = _tileArray + _tiling.getTileIndex(azimuth, altitude);
    cursor.firstInRow = _tileArray + _tiling.getTileIndex(0.0f, altitude);

    floodFill(cursor, TileSelection(*this, _tileArray, _tileArray + _tiling.getTileCount(), (TileSelection::Cursor*) _batchCountArray));

    this->glBatch(glm::value_ptr(matrix), prepareBatch((unsigned*) _batchOffs, _outIndexPos), alpha);
}

// renderer construction

void Renderer::prepareVertexData(InputVertices const& vertices, unsigned numStars, Tiling const& tiling) {

    size_t nTiles = tiling.getTileCount();
    size_t vertexIndex = 0u, currTileIndex = 0u, count_active = 0u;

    _tileArray[0].offset = 0u;
    _tileArray[0].flags = 0u;

    for (InputVertices::const_iterator i = vertices.begin(), e = vertices.end(); i != e; ++i) {
        size_t tileIndex = tiling.getTileIndex(i->getAzimuth(), i->getAltitude());
        assert(tileIndex >= currTileIndex);
        
        // moved on to another tile? -> flush
        if (tileIndex != currTileIndex) {

            Tile* tile = _tileArray + currTileIndex;
            Tile* lastTile = _tileArray + tileIndex;
            
            // set count of active vertices (upcoming lod)
            tile->count = count_active;
            // generate skipped, empty tiles
            for(size_t offset = vertexIndex; ++tile != lastTile ;) {
                tile->offset = offset, tile->count = 0u, tile->flags = 0u;
            }
                
            // initialize next (as far as possible here)
            lastTile->offset = vertexIndex;
            lastTile->flags = 0u;
            
            currTileIndex = tileIndex;
            count_active = 0u;
        }
        
        ++count_active;

        // write converted vertex
        _dataArray[vertexIndex++] = *i;
    }
    assert(vertexIndex == numStars);
    
    // flush last tile (see above)
    Tile* tile = _tileArray + currTileIndex;
    tile->count = count_active;
    for (Tile* e = _tileArray + nTiles + 1; ++tile != e;) {
        tile->offset = vertexIndex, tile->count = 0u, tile->flags = 0;
    }
}

bool Renderer::visitTile(Tile* tile) {
    unsigned index = tile - _tileArray;
    *_outIndexPos++ = index;

    return isTileVisible(index);
}

bool Renderer::isTileVisible(unsigned index) {

    float slice = _tiling.getSliceAngle();
    float halfSlice = 0.5f * slice;
    unsigned stride = _tiling.getAzimuthalTiles();
    float azimuth = (index % stride) * slice;
    float altitude = (index / stride) * slice - Radians::halfPi();
    float groundX = sin(azimuth);
    float groundZ = -cos(azimuth);
    float elevation = cos(altitude);
    vec3 tileCenter = vec3(groundX * elevation, sin(altitude), groundZ * elevation);
    float w = dot(_wRowVec, tileCenter);

    float daz = halfSlice * cos(std::max(0.0f, abs(altitude) - halfSlice));
    float dal = halfSlice;
    float adjustedNear = cos(_halfPerspectiveAngle + sqrt(daz * daz + dal * dal));
    
    return w >= adjustedNear;
}

unsigned Renderer::prepareBatch(unsigned const* indices, unsigned const* indicesEnd) {
    unsigned nRanges = 0u;
    GLint* offs = _batchOffs;
    GLsizei* count = _batchCountArray;

    for (unsigned* i = (unsigned*) _batchOffs; i != indicesEnd; ++i) {
        Tile* t = _tileArray + *i;
        if ((t->flags & Tile::render) > 0u && t->count > 0u) {
            *offs++ = t->offset;
            *count++ = t->count;
            ++nRanges;
        }
        t->flags = 0;
    }
    return nRanges;
}

// GL API handling

void Renderer::glAlloc() {
    GLchar const* const VERTEX_SHADER =
            "#version 120\n"
            "uniform float alpha;\n"
            "void main(void) {\n"
            "   vec3 c = gl_Color.rgb * 1.22;\n"
            "   float s = min(max(tan((c.r + c.g + c.b) / 3), 1.0), 3.0);\n"
            "   gl_Position = ftransform();\n"
            "   gl_FrontColor= gl_Color * alpha * 1.5;\n"
            "   gl_PointSize = s;\n"
            "}\n";

    _program.addShaderFromSourceCode(QGLShader::Vertex, VERTEX_SHADER);
    
    GLchar const* const FRAGMENT_SHADER =
            "#version 120\n"
            "void main(void) {\n"
            "   gl_FragColor = gl_Color;\n"
            "}\n";
    
    _program.addShaderFromSourceCode(QGLShader::Fragment, FRAGMENT_SHADER);
    _program.link();
    _alphaLocationHandle = _program.uniformLocation("alpha");

    glGenBuffersARB(1, & _vertexArrayHandle);
}

void Renderer::glFree() {
    glDeleteBuffersARB(1, & _vertexArrayHandle);
}

void Renderer::glUpload(GLsizei numStars) {
    glBindBufferARB(GL_ARRAY_BUFFER, _vertexArrayHandle);
    glBufferData(GL_ARRAY_BUFFER, numStars * sizeof(GpuVertex), _dataArray, GL_STATIC_DRAW);
    glBindBufferARB(GL_ARRAY_BUFFER, 0);
}

void Renderer::glBatch(GLfloat const* matrix, GLsizei n_ranges, float alpha) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // setup modelview matrix
    glPushMatrix();
    glLoadMatrixf(matrix);

    // set point size and smoothing + shader control
    glPointSize(1.0f);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    // select shader and vertex array
    _program.bind();
    _program.setUniformValue(_alphaLocationHandle, alpha);
    glBindBufferARB(GL_ARRAY_BUFFER, _vertexArrayHandle);
    glInterleavedArrays(GL_C4UB_V3F, sizeof(GpuVertex), 0l);
            
    // render
    glMultiDrawArrays(GL_POINTS, _batchOffs, _batchCountArray, n_ranges);

    // restore state
    glBindBufferARB(GL_ARRAY_BUFFER, 0);
    _program.release();
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glDisable(GL_POINT_SMOOTH);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    glPopMatrix();
}

// flood fill strategy

bool Renderer::TileSelection::select(Renderer::TileSelection::Cursor const& cursor) {
    Tile* tile = cursor.current;
    
    if (tile < _tileArray || tile >= _tilesEnd || !! (tile->flags & Tile::checked)) {
        // out of bounds or been here already
        return false;
    }
    
    // will check now and never again
    tile->flags |= Tile::checked;
    if (_rendererRef.visitTile(tile)) {
        // good one -> remember (for batching) and propagate
        tile->flags |= Tile::render;
        return true;
    }
    
    return false;
}

bool Renderer::TileSelection::process(Renderer::TileSelection::Cursor const& cursor) {
    Tile* tile = cursor.current;
    
    if (! (tile->flags & Tile::visited)) {
        tile->flags |= Tile::visited;
        return true;
    }
    
    return false;
}

void Renderer::TileSelection::right(Renderer::TileSelection::Cursor& cursor) const {
    cursor.current += 1;
    if (cursor.current == cursor.firstInRow + _rendererRef._tiling.getAzimuthalTiles()) {
        cursor.current = cursor.firstInRow;
    }
}

void Renderer::TileSelection::left(Renderer::TileSelection::Cursor& cursor) const {
    if (cursor.current == cursor.firstInRow) {
        cursor.current = cursor.firstInRow + _rendererRef._tiling.getAzimuthalTiles();
    }
    cursor.current -= 1;
}

void Renderer::TileSelection::up(Renderer::TileSelection::Cursor& cursor) const {
    unsigned numTiles = _rendererRef._tiling.getAzimuthalTiles();
    cursor.current += numTiles;
    cursor.firstInRow += numTiles;
}

void Renderer::TileSelection::down(Renderer::TileSelection::Cursor& cursor) const {
    unsigned numTiles = _rendererRef._tiling.getAzimuthalTiles();
    cursor.current -= numTiles;
    cursor.firstInRow -= numTiles;
}

void Renderer::TileSelection::defer(Renderer::TileSelection::Cursor const& cursor) {
    *_stackPos++ = cursor;
}

bool Renderer::TileSelection::deferred(Renderer::TileSelection::Cursor& cursor) {
    if (_stackPos != _stackArray) {
        cursor = *--_stackPos;
        return true;
    }
    return false;
}
