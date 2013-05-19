//
// starfield/renderer/Renderer.h
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__renderer__Renderer__
#define __interface__starfield__renderer__Renderer__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

#include "starfield/Config.h"
#include "starfield/data/InputVertex.h"
#include "starfield/data/BrightnessLevel.h"

#include "starfield/data/Tile.h"
#include "starfield/data/GpuVertex.h"

#include "Tiling.h"

//
// FOV culling
// ===========
//
// As stars can be thought of as at infinity distance, the field of view only
// depends on perspective and rotation:
//
//                                 _----_  <-- visible stars
//             from above         +-near-+ -  -
//                                 \    /     |
//                near width:       \  /      | cos(p/2)
//                     2sin(p/2)     \/       _
//                                  center    
//
//
// Now it is important to note that a change in altitude maps uniformly to a 
// distance on a sphere. This is NOT the case for azimuthal angles: In this
// case a factor of 'cos(alt)' (the orbital radius) applies: 
//
//
//             |<-cos alt ->|    | |<-|<----->|->| d_azi cos(alt)
//                               |
//                      __--*    |    ---------   -
//                  __--     *   |   |         |  ^ d_alt 
//              __-- alt)    *   |  |           | v 
//             --------------*-  |  ------------- -
//                               |
//             side view         | tile on sphere
//
//
// This lets us find a worst-case (Eigen) angle from the center to the edge
// of a tile as
//
//     hypot( 0.5 d_alt, 0.5 d_azi cos(alt_absmin) ).
//
// This angle must be added to 'p' (the perspective angle) in order to find
// an altered near plane for the culling decision.
//

namespace starfield {

    class Renderer {
    public:

        Renderer(InputVertices const& src,
                 size_t n,
                 unsigned k,
                 BrightnessLevel b,
                 BrightnessLevel bMin) :

            _dataArray(0l), 
            _tileArray(0l), 
            _tiling(k) {

            this->glAlloc();

            Tiling tiling(k);
            size_t nTiles = tiling.getTileCount();

            // REVISIT: could coalesce allocation for faster rebuild
            // REVISIT: batch arrays are probably oversized, but - hey - they
            // are not very large (unless for insane tiling) and we're better
            // off safe than sorry
            _dataArray = new GpuVertex[n];
            _tileArray = new Tile[nTiles + 1];
            _batchOffs = new GLint[nTiles * 2]; 
            _batchCountArray = new GLsizei[nTiles * 2];

            prepareVertexData(src, n, tiling, b, bMin);

            this->glUpload(n);
        }

        ~Renderer() {

            delete[] _dataArray;
            delete[] _tileArray;
            delete[] _batchCountArray;
            delete[] _batchOffs;

            this->glFree();
        }

        void render(float perspective,
                    float aspect,
                    mat4 const& orientation,
                    BrightnessLevel minBright,
                    float alpha) {

// printLog("
//      Stars.cpp: rendering at minimal brightness %d\n", minBright);

            float halfPersp = perspective * 0.5f;

            // define diagonal and near distance
            float halfDiag = std::sin(halfPersp);
            float nearClip = std::cos(halfPersp);
            
            // determine half dimensions based on the screen diagonal
            //
            //  ww + hh = dd
            //  a = w / h => w = ha
            //  hh + hh aa = dd
            //  hh = dd / (1 + aa)
            float hh = sqrt(halfDiag * halfDiag / (1.0f + aspect * aspect));
            float hw = hh * aspect;

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
            altitude = glm::clamp(altitude,
                                  -Radians::halfPi() + eps, Radians::halfPi() - eps);
#if STARFIELD_HEMISPHERE_ONLY
            altitude = std::max(0.0f, altitude);
#endif

#if STARFIELD_DEBUG_CULLING
            mat4 matrix_debug = glm::translate(glm::frustum(-hw, hw, -hh, hh, nearClip, 10.0f), 
                                               vec3(0.0f, 0.0f, -4.0f)) *
                    glm::affineInverse(matrix);
#endif

            matrix = glm::frustum(-hw,hw, -hh,hh, nearClip,10.0f) * glm::affineInverse(matrix); 

            this->_outIndexPos = (unsigned*) _batchOffs;
            this->_wRowVec = vec3(row(matrix, 3));
            this->_halfPerspectiveAngle = halfPersp;
            this->_minBright = minBright;

            TileSelection::Cursor cursor;
            cursor.current = _tileArray + _tiling.getTileIndex(azimuth, altitude);
            cursor.firstInRow = _tileArray + _tiling.getTileIndex(0.0f, altitude);

            floodFill(cursor, TileSelection(*this, _tileArray, _tileArray + _tiling.getTileCount(),
                                            (TileSelection::Cursor*) _batchCountArray));

#if STARFIELD_DEBUG_CULLING
#   define matrix matrix_debug
#endif
            this->glBatch(glm::value_ptr(matrix), prepareBatch(
                    (unsigned*) _batchOffs, _outIndexPos), alpha);

#if STARFIELD_DEBUG_CULLING
#   undef matrix
#endif
        }

    private:
        // renderer construction

        void prepareVertexData(InputVertices const& src,
                               size_t n, // <-- at bMin and brighter
                               Tiling const& tiling, 
                               BrightnessLevel b, 
                               BrightnessLevel bMin) {

            size_t nTiles = tiling.getTileCount();
            size_t vertexIndex = 0u, currTileIndex = 0u, count_active = 0u;

            _tileArray[0].offset = 0u;
            _tileArray[0].lod = b;
            _tileArray[0].flags = 0u;

            for (InputVertices::const_iterator i = 
                    src.begin(), e = src.end(); i != e; ++i) {

                BrightnessLevel bv = getBrightness(i->getColor());
                // filter by alloc brightness
                if (bv >= bMin) {

                    size_t tileIndex = tiling.getTileIndex(
                            i->getAzimuth(), i->getAltitude());

                    assert(tileIndex >= currTileIndex);

                    // moved on to another tile? -> flush
                    if (tileIndex != currTileIndex) {

                        Tile* t = _tileArray + currTileIndex;
                        Tile* tLast = _tileArray + tileIndex;

                        // set count of active vertices (upcoming lod)
                        t->count = count_active;
                        // generate skipped, empty tiles
                        for(size_t offs = vertexIndex; ++t != tLast ;) {
                            t->offset = offs, t->count = 0u, 
                            t->lod = b, t->flags = 0u;
                        }

                        // initialize next (as far as possible here)
                        tLast->offset = vertexIndex;
                        tLast->lod = b;
                        tLast->flags = 0u;

                        currTileIndex = tileIndex;
                        count_active = 0u;
                    }

                    if (bv >= b)
                        ++count_active;

// printLog("Stars.cpp: Vertex %d on tile #%d\n", vertexIndex, tileIndex);

                    // write converted vertex
                    _dataArray[vertexIndex++] = *i;
                }
            }
            assert(vertexIndex == n);
            // flush last tile (see above)
            Tile* t = _tileArray + currTileIndex; 
            t->count = count_active;
            for (Tile* e = _tileArray + nTiles + 1; ++t != e;) {
                t->offset = vertexIndex, t->count = 0u,
                t->lod = b, t->flags = 0;
            }
        }

        // FOV culling / LOD

        class TileSelection;
        friend class Renderer::TileSelection;

        class TileSelection {

        public:
            struct Cursor { Tile* current, * firstInRow; };
        private:
            Renderer&           _rendererRef;
            Cursor*     const   _stackArray;
            Cursor*             _stackPos;
            Tile const* const   _tileArray;
            Tile const* const   _tilesEnd;

        public:

            TileSelection(Renderer& renderer, Tile const* tiles, 
                          Tile const* tiles_end, Cursor* stack) :

                _rendererRef(renderer),
                _stackArray(stack),
                _stackPos(stack), 
                _tileArray(tiles), 
                _tilesEnd(tiles_end) {
            }
     
        protected:

            // flood fill strategy

            bool select(Cursor const& c) {
                Tile* t = c.current;

                if (t < _tileArray || t >= _tilesEnd ||
                        !! (t->flags & Tile::checked)) {

                    // out of bounds or been here already
                    return false; 
                }

                // will check now and never again
                t->flags |= Tile::checked;
                if (_rendererRef.visitTile(t)) {

                    // good one -> remember (for batching) and propagate
                    t->flags |= Tile::render;
                    return true;
                }
                return false;
            }

            bool process(Cursor const& c) { 
                Tile* t = c.current;

                if (! (t->flags & Tile::visited)) {

                    t->flags |= Tile::visited;
                    return true;
                }
                return false;
            }

            void right(Cursor& c) const {

                c.current += 1;
                if (c.current == c.firstInRow + _rendererRef._tiling.getAzimuthalTiles()) {
                    c.current = c.firstInRow;
                }
            }
            void left(Cursor& c)  const {
 
                if (c.current == c.firstInRow) {
                    c.current = c.firstInRow + _rendererRef._tiling.getAzimuthalTiles();
                }
                c.current -= 1;
            }
            void up(Cursor& c) const { 

                unsigned d = _rendererRef._tiling.getAzimuthalTiles();
                c.current += d;
                c.firstInRow += d;
            }
            void down(Cursor& c)  const {

                unsigned d = _rendererRef._tiling.getAzimuthalTiles();
                c.current -= d;
                c.firstInRow -= d;
            }

            void defer(Cursor const& t) {

                *_stackPos++ = t;
            }

            bool deferred(Cursor& cursor) {

                if (_stackPos != _stackArray) {
                    cursor = *--_stackPos;
                    return true;
                }
                return false;
            }
        };

        bool visitTile(Tile* t) {

            unsigned index =  t - _tileArray;
            *_outIndexPos++ = index;

            if (! tileVisible(t, index)) {
                return false;
            }

            if (t->lod != _minBright) {
                updateVertexCount(t, _minBright);
            }
            return true;
        }

        bool tileVisible(Tile* t, unsigned i) {

            float slice = _tiling.getSliceAngle();
            float halfSlice = 0.5f * slice;
            unsigned stride = _tiling.getAzimuthalTiles();
            float azimuth = (i % stride) * slice;
            float altitude = (i / stride) * slice - Radians::halfPi();
            float gx =  sin(azimuth);
            float gz = -cos(azimuth);
            float exz = cos(altitude);
            vec3 tileCenter = vec3(gx * exz, sin(altitude), gz * exz);
            float w = dot(_wRowVec, tileCenter);

            float daz = halfSlice * cos(std::max(0.0f, abs(altitude) - halfSlice));
            float dal = halfSlice;
            float adjustedNear = cos(_halfPerspectiveAngle + sqrt(daz * daz + dal * dal));

// printLog("Stars.cpp: checking tile #%d, w = %f, near = %f\n", i,  w, nearClip);

            return w >= adjustedNear;
        }

        void updateVertexCount(Tile* t, BrightnessLevel minBright) {

            // a growing number of stars needs to be rendereed when the 
            // minimum brightness decreases
            // perform a binary search in the so found partition for the
            // new vertex count of this tile

            GpuVertex const* start = _dataArray + t[0].offset;
            GpuVertex const* end = _dataArray + t[1].offset;

            assert(end >= start);

            if (start == end)
                return;

            if (t->lod < minBright)
                end = start + t->count;
            else
                start += (t->count > 0 ? t->count - 1 : 0);

            end = std::upper_bound(
                    start, end, minBright, GreaterBrightness());

            assert(end >= _dataArray + t[0].offset);

            t->count = end - _dataArray - t[0].offset; 
            t->lod = minBright;
        }

        unsigned prepareBatch(unsigned const* indices, 
                              unsigned const* indicesEnd) {

            unsigned nRanges = 0u;
            GLint* offs = _batchOffs;
            GLsizei* count = _batchCountArray;

            for (unsigned* i = (unsigned*) _batchOffs; 
                    i != indicesEnd; ++i) {

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

        void glAlloc() {

            GLchar const* const VERTEX_SHADER =
                    "#version 120\n"
                    "uniform float alpha;\n"
                    "void main(void) {\n"

                    "   vec3 c = gl_Color.rgb * 1.0125;\n"
                    "   float s = max(1.0, dot(c, c) * 0.7);\n"

                    "   gl_Position = ftransform();\n"
                    "   gl_FrontColor= gl_Color * alpha;\n"
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

        void glFree() {

            glDeleteBuffersARB(1, & _vertexArrayHandle);
        }

        void glUpload(GLsizei n) {
            glBindBufferARB(GL_ARRAY_BUFFER, _vertexArrayHandle);
            glBufferData(GL_ARRAY_BUFFER,
                    n * sizeof(GpuVertex), _dataArray, GL_STATIC_DRAW);
            //glInterleavedArrays(GL_C4UB_V3F, sizeof(GpuVertex), 0l);

            glBindBufferARB(GL_ARRAY_BUFFER, 0); 
        }

        void glBatch(GLfloat const* matrix, GLsizei n_ranges, float alpha) {

// printLog("Stars.cpp: rendering %d-multibatch\n", n_ranges);

// for (int i = 0; i < n_ranges; ++i)
//     printLog("Stars.cpp: Batch #%d - %d stars @ %d\n", i, 
//             _batchOffs[i], _batchCountArray[i]);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_LIGHTING);

            // setup modelview matrix (identity)
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            // set projection matrix
            glMatrixMode(GL_PROJECTION);
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
            glMultiDrawArrays(GL_POINTS,
                    _batchOffs, _batchCountArray, n_ranges);

            // restore state
            glBindBufferARB(GL_ARRAY_BUFFER, 0); 
            _program.release();
            glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
            glDisable(GL_POINT_SMOOTH);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }

        // variables

        GpuVertex*      _dataArray;
        Tile*           _tileArray;
        GLint*          _batchOffs;
        GLsizei*        _batchCountArray;
        GLuint          _vertexArrayHandle;
        ProgramObject   _program;
        int             _alphaLocationHandle;

        Tiling          _tiling;

        unsigned*       _outIndexPos;
        vec3            _wRowVec;
        float           _halfPerspectiveAngle;
        BrightnessLevel _minBright;

    };

} // anonymous namespace

#endif

