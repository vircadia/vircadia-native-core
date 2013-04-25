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

        GpuVertex*      _arrData;
        Tile*           _arrTile;
        GLint*          _arrBatchOffs;
        GLsizei*        _arrBatchCount;
        GLuint          _hndVertexArray;
        OGlProgram      _objProgram;

        Tiling          _objTiling;

        unsigned*       _itrOutIndex;
        vec3            _vecWxform;
        float           _valHalfPersp;
        BrightnessLevel _valMinBright;

    public:

        Renderer(InputVertices const& src,
                 size_t n,
                 unsigned k,
                 BrightnessLevel b,
                 BrightnessLevel bMin) :

            _arrData(0l), 
            _arrTile(0l), 
            _objTiling(k) {

            this->glAlloc();

            Tiling tiling(k);
            size_t nTiles = tiling.getTileCount();

            // REVISIT: could coalesce allocation for faster rebuild
            // REVISIT: batch arrays are probably oversized, but - hey - they
            // are not very large (unless for insane tiling) and we're better
            // off safe than sorry
            _arrData = new GpuVertex[n];
            _arrTile = new Tile[nTiles + 1];
            _arrBatchOffs = new GLint[nTiles * 2]; 
            _arrBatchCount = new GLsizei[nTiles * 2];

            prepareVertexData(src, n, tiling, b, bMin);

            this->glUpload(n);
        }

        ~Renderer() {

            delete[] _arrData;
            delete[] _arrTile;
            delete[] _arrBatchCount;
            delete[] _arrBatchOffs;

            this->glFree();
        }

        void render(float perspective,
                    float aspect,
                    mat4 const& orientation,
                    BrightnessLevel minBright) {

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

            this->_itrOutIndex = (unsigned*) _arrBatchOffs;
            this->_vecWxform = vec3(row(matrix, 3));
            this->_valHalfPersp = halfPersp;
            this->_valMinBright = minBright;

            TileSelection::Cursor cursor;
            cursor.current = _arrTile + _objTiling.getTileIndex(azimuth, altitude);
            cursor.firstInRow = _arrTile + _objTiling.getTileIndex(0.0f, altitude);

            floodFill(cursor, TileSelection(*this, _arrTile, _arrTile + _objTiling.getTileCount(),
                                            (TileSelection::Cursor*) _arrBatchCount));

#if STARFIELD_DEBUG_CULLING
#   define matrix matrix_debug
#endif
            this->glBatch(glm::value_ptr(matrix), prepareBatch(
                    (unsigned*) _arrBatchOffs, _itrOutIndex) );

#if STARFIELD_DEBUG_CULLING
#   undef matrix
#endif
        }

    private: // renderer construction

        void prepareVertexData(InputVertices const& src,
                               size_t n, // <-- at bMin and brighter
                               Tiling const& tiling, 
                               BrightnessLevel b, 
                               BrightnessLevel bMin) {

            size_t nTiles = tiling.getTileCount();
            size_t vertexIndex = 0u, currTileIndex = 0u, count_active = 0u;

            _arrTile[0].offset = 0u;
            _arrTile[0].lod = b;
            _arrTile[0].flags = 0u;

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

                        Tile* t = _arrTile + currTileIndex;
                        Tile* tLast = _arrTile + tileIndex;

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
                    _arrData[vertexIndex++] = *i;
                }
            }
            assert(vertexIndex == n);
            // flush last tile (see above)
            Tile* t = _arrTile + currTileIndex; 
            t->count = count_active;
            for (Tile* e = _arrTile + nTiles + 1; ++t != e;) {
                t->offset = vertexIndex, t->count = 0u,
                t->lod = b, t->flags = 0;
            }
        }


    private: // FOV culling / LOD

        class TileSelection;
        friend class Renderer::TileSelection;

        class TileSelection {

        public:
            struct Cursor { Tile* current, * firstInRow; };
        private:
            Renderer&           _refRenderer;
            Cursor*     const   _arrStack;
            Cursor*             _itrStack;
            Tile const* const   _arrTile;
            Tile const* const   _ptrTilesEnd;

        public:

            TileSelection(Renderer& renderer, Tile const* tiles, 
                          Tile const* tiles_end, Cursor* stack) :

                _refRenderer(renderer),
                _arrStack(stack),
                _itrStack(stack), 
                _arrTile(tiles), 
                _ptrTilesEnd(tiles_end) {
            }
     
        protected:

            // flood fill strategy

            bool select(Cursor const& c) {
                Tile* t = c.current;

                if (t < _arrTile || t >= _ptrTilesEnd ||
                        !! (t->flags & Tile::checked)) {

                    // out of bounds or been here already
                    return false; 
                }

                // will check now and never again
                t->flags |= Tile::checked;
                if (_refRenderer.visitTile(t)) {

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
                if (c.current == c.firstInRow + _refRenderer._objTiling.getAzimuthalTiles()) {
                    c.current = c.firstInRow;
                }
            }
            void left(Cursor& c)  const {
 
                if (c.current == c.firstInRow) {
                    c.current = c.firstInRow + _refRenderer._objTiling.getAzimuthalTiles();
                }
                c.current -= 1;
            }
            void up(Cursor& c) const { 

                unsigned d = _refRenderer._objTiling.getAzimuthalTiles();
                c.current += d;
                c.firstInRow += d;
            }
            void down(Cursor& c)  const {

                unsigned d = _refRenderer._objTiling.getAzimuthalTiles();
                c.current -= d;
                c.firstInRow -= d;
            }

            void defer(Cursor const& t) {

                *_itrStack++ = t;
            }

            bool deferred(Cursor& cursor) {

                if (_itrStack != _arrStack) {
                    cursor = *--_itrStack;
                    return true;
                }
                return false;
            }
        };

        bool visitTile(Tile* t) {

            unsigned index =  t - _arrTile;
            *_itrOutIndex++ = index;

            if (! tileVisible(t, index)) {
                return false;
            }

            if (t->lod != _valMinBright) {
                updateVertexCount(t, _valMinBright);
            }
            return true;
        }

        bool tileVisible(Tile* t, unsigned i) {

            float slice = _objTiling.getSliceAngle();
            float halfSlice = 0.5f * slice;
            unsigned stride = _objTiling.getAzimuthalTiles();
            float azimuth = (i % stride) * slice;
            float altitude = (i / stride) * slice - Radians::halfPi();
            float gx =  sin(azimuth);
            float gz = -cos(azimuth);
            float exz = cos(altitude);
            vec3 tileCenter = vec3(gx * exz, sin(altitude), gz * exz);
            float w = dot(_vecWxform, tileCenter);

            float daz = halfSlice * cos(std::max(0.0f, abs(altitude) - halfSlice));
            float dal = halfSlice;
            float adjustedNear = cos(_valHalfPersp + sqrt(daz * daz + dal * dal));

// printLog("Stars.cpp: checking tile #%d, w = %f, near = %f\n", i,  w, nearClip);

            return w >= adjustedNear;
        }

        void updateVertexCount(Tile* t, BrightnessLevel minBright) {

            // a growing number of stars needs to be rendereed when the 
            // minimum brightness decreases
            // perform a binary search in the so found partition for the
            // new vertex count of this tile

            GpuVertex const* start = _arrData + t[0].offset;
            GpuVertex const* end = _arrData + t[1].offset;

            assert(end >= start);

            if (start == end)
                return;

            if (t->lod < minBright)
                end = start + t->count;
            else
                start += (t->count > 0 ? t->count - 1 : 0);

            end = std::upper_bound(
                    start, end, minBright, GreaterBrightness());

            assert(end >= _arrData + t[0].offset);

            t->count = end - _arrData - t[0].offset; 
            t->lod = minBright;
        }

        unsigned prepareBatch(unsigned const* indices, 
                              unsigned const* indicesEnd) {

            unsigned nRanges = 0u;
            GLint* offs = _arrBatchOffs;
            GLsizei* count = _arrBatchCount;

            for (unsigned* i = (unsigned*) _arrBatchOffs; 
                    i != indicesEnd; ++i) {

                Tile* t = _arrTile + *i;
                if ((t->flags & Tile::render) > 0u && t->count > 0u) {

                    *offs++ = t->offset;
                    *count++ = t->count;
                    ++nRanges;
                }
                t->flags = 0;
            }
            return nRanges;
        }

    private: // gl API handling 

#ifdef __APPLE__ 
#   define glBindVertexArray glBindVertexArrayAPPLE 
#   define glGenVertexArrays glGenVertexArraysAPPLE 
#   define glDeleteVertexArrays glDeleteVertexArraysAPPLE 
#endif
        void glAlloc() {

            GLchar const* const VERTEX_SHADER =
                    "#version 120\n"
                    "void main(void) {\n"

                    "   vec3 c = gl_Color.rgb * 1.0125;\n"
                    "   float s = max(1.0, dot(c, c) * 0.7);\n"

                    "   gl_Position = ftransform();\n"
                    "   gl_FrontColor= gl_Color;\n"
                    "   gl_PointSize = s;\n"
                    "}\n";

            _objProgram.addShader(GL_VERTEX_SHADER, VERTEX_SHADER);
            GLchar const* const FRAGMENT_SHADER =
                    "#version 120\n"
                    "void main(void) {\n"
                    "   gl_FragColor = gl_Color;\n"
                    "}\n";
            _objProgram.addShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
            _objProgram.link();

            glGenVertexArrays(1, & _hndVertexArray);
        }

        void glFree() {

            glDeleteVertexArrays(1, & _hndVertexArray);
        }

        void glUpload(GLsizei n) {

            GLuint vbo;
            glGenBuffers(1, & vbo);

            glBindVertexArray(_hndVertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER,
                    n * sizeof(GpuVertex), _arrData, GL_STATIC_DRAW);
            glInterleavedArrays(GL_C4UB_V3F, sizeof(GpuVertex), 0l);

            glBindVertexArray(0); 
        }

        void glBatch(GLfloat const* matrix, GLsizei n_ranges) {

// printLog("Stars.cpp: rendering %d-multibatch\n", n_ranges);

// for (int i = 0; i < n_ranges; ++i)
//     printLog("Stars.cpp: Batch #%d - %d stars @ %d\n", i, 
//             _arrBatchOffs[i], _arrBatchCount[i]);

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
            _objProgram.activate();
            glBindVertexArray(_hndVertexArray);

            // render
            glMultiDrawArrays(GL_POINTS,
                    _arrBatchOffs, _arrBatchCount, n_ranges);

            // restore state
            glBindVertexArray(0); 
            glUseProgram(0);
            glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
            glDisable(GL_POINT_SMOOTH);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }
#ifdef __APPLE__
#   undef glBindVertexArray 
#   undef glGenVertexArrays 
#   undef glDeleteVertexArrays
#endif
    };

} // anonymous namespace

#endif

