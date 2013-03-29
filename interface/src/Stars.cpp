//
// Stars.cpp
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "InterfaceConfig.h"

#include "Stars.h"
#include "UrlReader.h"
#include "FieldOfView.h"
#include "AngleUtils.h"
#include "Radix2InplaceSort.h"
#include "Radix2IntegerScanner.h"
#include "FloodFill.h"

#include <stddef.h>
#include <stdint.h>
#include <float.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include <new>
#include <vector>
#include <memory>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/swizzle.hpp>

/* Data pipeline
 * =============
 *
 * ->> readInput -(load)--+---- (get brightness & sort) ---> brightness LUT
 *                        |              |        
 * ->> setResolution --+  |             >extractBrightnessLevels<
 *                     V  |
 *            (sort by (tile,brightness))
 *                        |         |
 * ->> setLOD  ---+       |    >retile<   ->> setLOD --> (just parameterize
 *                V       V                          when enough data on-GPU)
 *          (filter by max-LOD brightness,         
 *            build tile info for rendering)
 *                        |             |
 *                        V         >recreateRenderer<
 *                 (set new renderer)/
 *
 *
 * (process), ->> entry point, ---> data flow, >internal routine<
 *
 *
 * FOV culling
 * ===========
 *
 * As stars can be thought of as at infinity distance, the field of view only
 * depends on perspective and rotation:
 *
 *                                 _----_  <-- visible stars
 *             from above         +-near-+ -  -
 *                                 \    /     |
 *                near width:       \  /      | cos(p/2)
 *                     2sin(p/2)     \/       _
 *                                  center    
 *
 *
 * Now it is important to note that a change in altitude maps uniformly to a 
 * distance on a sphere. This is NOT the case for azimuthal angles: In this
 * case a factor of 'cos(alt)' (the orbital radius) applies: 
 *
 *
 *             |<-cos alt ->|    | |<-|<----->|->| d_azi cos(alt)
 *                               |
 *                      __--*    |    ---------   -
 *                  __--     *   |   |         |  ^ d_alt 
 *              __-- alt)    *   |  |           | v 
 *             --------------*-  |  ------------- -
 *                               |
 *             side view         | tile on sphere
 *
 *
 * This lets us find a worst case (Eigen) angle from the center to the edge
 * of a tile as
 *
 *     hypot( 0.5 d_alt, 0.5 d_azi cos(alt_absmin) ).
 *
 * This angle must be added to 'p' (the perspective angle) in order to find
 * an altered near plane for the culling decision.
 *
 *
 * Still open 
 * ==========
 *
 * o atomics/mutexes need to be added as annotated in the source to allow
 *   concurrent threads to pull the strings to e.g. have a low priority
 *   thread run the data pipeline for update -- rendering is wait-free
 */

// Compile time configuration

// #define FORCE_POSITIVE_ALTITUDE // uncomment for hemisphere only
// #define SAVE_MEMORY // uncomment not to use 16-bit types
// #define SEE_LOD // uncomment to peek behind the scenes

namespace
{
    using std::swap;
    using std::min;
    using std::max;

    using glm::vec3;
    using glm::vec4;
    using glm::dot;
    using glm::normalize;
    using glm::swizzle;
    using glm::X;
    using glm::Y;
    using glm::Z;
    using glm::W;
    using glm::mat4;
    using glm::column;
    using glm::row;

#ifdef SAVE_MEMORY
    typedef uint16_t nuint;
    typedef uint32_t wuint;
#else
    typedef uint32_t nuint;
    typedef uint64_t wuint;
#endif

    class InputVertex {

        unsigned    val_color;
        float       val_azimuth;
        float       val_altitude;
    public:

        InputVertex(float azimuth, float altitude, unsigned color) {

            val_color = color >> 16 & 0xffu | color & 0xff00u |
                    color << 16 & 0xff0000u | 0xff000000u;

            azimuth = angleConvert<Degrees,Radians>(azimuth);
            altitude = angleConvert<Degrees,Radians>(altitude);

            angleHorizontalPolar<Radians>(azimuth, altitude);

            val_azimuth = azimuth;
            val_altitude = altitude;
        }

        float getAzimuth() const { return val_azimuth; }
        float getAltitude() const { return val_altitude; }
        unsigned getColor() const { return val_color; } 
    }; 

    typedef std::vector<InputVertex> InputVertices;

    typedef nuint BrightnessLevel;
#ifdef SAVE_MEMORY
    const unsigned BrightnessBits = 16u;
#else
    const unsigned BrightnessBits = 18u;
#endif
    const BrightnessLevel BrightnessMask = (1u << (BrightnessBits)) - 1u;
    
    typedef std::vector<BrightnessLevel> BrightnessLevels;

    BrightnessLevel getBrightness(unsigned c) {

        unsigned r = (c >> 16) & 0xff;
        unsigned g = (c >> 8) & 0xff;
        unsigned b = c & 0xff;
#ifdef SAVE_MEMORY
        return BrightnessLevel((r*r+g*g+b*b) >> 2);
#else
        return BrightnessLevel(r*r+g*g+b*b);
#endif
    }

    struct BrightnessSortScanner : Radix2IntegerScanner<BrightnessLevel> {

        typedef Radix2IntegerScanner<BrightnessLevel> base;

        BrightnessSortScanner() : base(BrightnessBits) { }

        bool bit(BrightnessLevel const& k, state_type& s) { 

            // bit is inverted to achieve descending order
            return ! base::bit(k,s);
        }
    };

    void extractBrightnessLevels(BrightnessLevels& dst, 
                                 InputVertices const& src) {
        dst.clear();
        dst.reserve(src.size());
        for (InputVertices::const_iterator i = 
                src.begin(), e = src.end(); i != e; ++i)
            dst.push_back( getBrightness(i->getColor()) );

        radix2InplaceSort(dst.begin(), dst.end(), BrightnessSortScanner());
    }

    class Tiling {

        unsigned    val_k;
        float       val_rcp_slice;
        unsigned    val_bits;

    public:

        Tiling(unsigned k) : 
            val_k(k),
            val_rcp_slice(k / Radians::twice_pi()) {
            val_bits = ceil(log2(getTileCount()) + 2);
        }

        unsigned getAzimuthalTiles() const { return val_k; }
        unsigned getAltitudinalTiles() const { return val_k / 2 + 1; }
        unsigned getTileIndexBits() const { return val_bits; }

        unsigned getTileCount() const {
            return getAzimuthalTiles() * getAltitudinalTiles();
        }

        unsigned getTileIndex(float azimuth, float altitude) const {
            return discreteAzimuth(azimuth) +
                    val_k * discreteAltitude(altitude);
        }

        unsigned getTileIndex(InputVertex const& v) const {
            return getTileIndex(v.getAzimuth(), v.getAltitude());
        }

        float getSliceAngle() const {
            return 1.0f / val_rcp_slice;
        }

    private:

        unsigned discreteAngle(float unsigned_angle) const {
            return unsigned(round(unsigned_angle * val_rcp_slice));
        }

        unsigned discreteAzimuth(float a) const {
            return discreteAngle(a) % val_k;
        }

        unsigned discreteAltitude(float a) const {
            return min(getAltitudinalTiles() - 1, 
                    discreteAngle(a + Radians::half_pi()) );
        }

    };

    class TileSortScanner : public Radix2IntegerScanner<unsigned>
    {
            Tiling obj_tiling;

            typedef Radix2IntegerScanner<unsigned> Base;
        public:

            explicit TileSortScanner(Tiling const& tiling) :

                Base(tiling.getTileIndexBits() + BrightnessBits),
                obj_tiling(tiling) {
            }

            bool bit(InputVertex const& v, state_type const& s) const {

                // inspect (tile_index, brightness) tuples
                unsigned key = getBrightness(v.getColor()) ^ BrightnessMask;
                key |= obj_tiling.getTileIndex(v) << BrightnessBits;
                return Base::bit(key, s); 
            }
    };

    struct Tile {

        nuint           offset;
        nuint           count; 
        BrightnessLevel lod;
        uint16_t        flags;

        static uint16_t const checked = 1;
        static uint16_t const visited = 2;
        static uint16_t const render = 4;
    };


    class GpuVertex {

        unsigned    val_color;
        float       val_x;
        float       val_y;
        float       val_z;
    public:

        GpuVertex() { }

        GpuVertex(InputVertex const& in) {

            val_color = in.getColor();
            float azi = in.getAzimuth();
            float alt = in.getAltitude();

            // ground vector in x/z plane...
            float gx =  sin(azi);
            float gz = -cos(azi);

            // ...elevated in y direction by altitude
            float exz = cos(alt);
            val_x = gx * exz;
            val_y = sin(alt);
            val_z = gz * exz;
        }

        unsigned getColor() const { return val_color; } 
    };

    struct GreaterBrightness {

        bool operator()(InputVertex const& lhs, InputVertex const& rhs) const {
            return getBrightness(lhs.getColor())
                    > getBrightness(rhs.getColor());
        }
        bool operator()(BrightnessLevel lhs, GpuVertex const& rhs) const {
            return lhs > getBrightness(rhs.getColor());;
        }
        bool operator()(BrightnessLevel lhs, BrightnessLevel rhs) const {
            return lhs > rhs;
        }
    };

    class Loader : UrlReader
    {
        InputVertices*  ptr_vertices;
        unsigned        val_limit;

        unsigned        val_lineno;
        char const*     str_actual_url;

        unsigned        val_records_read;
        BrightnessLevel val_min_brightness;
    public:

        bool loadVertices(
                InputVertices& destination, char const* url, unsigned limit)
        {
            ptr_vertices = & destination;
            val_limit = limit;
#ifdef SAVE_MEMORY
            if (val_limit == 0 || val_limit > 60000u)
                val_limit = 60000u;
#endif
            str_actual_url = url; // in case we fail early

            if (! UrlReader::readUrl(url, *this))
            {
                fprintf(stderr, "%s:%d: %s\n",
                        str_actual_url, val_lineno, getError());

                return false;
            }
 fprintf(stderr, "Stars.cpp: read %d vertices, using %d\n", 
      val_records_read, ptr_vertices->size());

            return true;
        }

    protected:

        friend class UrlReader;

        void begin(char const* url,
                   char const* type,
                   int64_t size,
                   int64_t stardate) {

            val_lineno = 0u;
            str_actual_url = url; // new value in http redirect

            val_records_read = 0u;

            ptr_vertices->clear();
            ptr_vertices->reserve(val_limit);
// fprintf(stderr, "Stars.cpp: loader begin %s\n", url);
        }
        
        size_t transfer(char* input, size_t bytes) {

            size_t consumed = 0u;
            char const* end = input + bytes;
            char* line, * next = input;

            for (;;) {

                // advance to next line
                for (; next != end && isspace(*next); ++next);
                consumed = next - input;
                line = next;
                ++val_lineno;
                for (; next != end && *next != '\n' && *next != '\r'; ++next);
                if (next == end)
                    return consumed;
                *next++ = '\0';

                // skip comments
                if (*line == '\\' || *line == '/' || *line == ';')
                    continue;

                // parse
                float azi, alt;
                unsigned c;
                if (sscanf(line, " %f %f #%x", & azi, & alt, & c) == 3) {

                    if (spaceFor( getBrightness(c) )) {

                        storeVertex(azi, alt, c);
                    }

                    ++val_records_read;

                } else {

                    fprintf(stderr, "Stars.cpp:%d: Bad input from %s\n", 
                            val_lineno, str_actual_url);
                }

            }
            return consumed;
        }

        void end(bool ok)
        { }

    private:

        bool atLimit() { return val_limit > 0u && val_records_read >= val_limit; }

        bool spaceFor(BrightnessLevel b) {

            if (! atLimit()) {
                return true;
            }

            // just reached the limit? -> establish a minimum heap and 
            // remember the brightness at its top
            if (val_records_read == val_limit) {

// fprintf(stderr, "Stars.cpp: vertex limit reached -> heap mode\n");

                std::make_heap(
                    ptr_vertices->begin(), ptr_vertices->end(),
                    GreaterBrightness() );

                val_min_brightness = getBrightness(
                        ptr_vertices->begin()->getColor() );
            }

            // not interested? say so
            if (val_min_brightness >= b)
                return false;

            // otherwise free up space for the new vertex
            std::pop_heap(
                ptr_vertices->begin(), ptr_vertices->end(),
                GreaterBrightness() );
            ptr_vertices->pop_back();
            return true;
        }

        void storeVertex(float azi, float alt, unsigned color) {
  
            ptr_vertices->push_back(InputVertex(azi, alt, color));

            if (atLimit()) {

                std::push_heap(
                    ptr_vertices->begin(), ptr_vertices->end(),
                    GreaterBrightness() );

                val_min_brightness = getBrightness(
                        ptr_vertices->begin()->getColor() );
            }
        }
    };

    class Renderer;

    class TileCulling {

            Renderer&           ref_renderer;
            Tile** const        arr_stack;
            Tile**              itr_stack;
            Tile const* const   arr_tile;
            Tile const* const   itr_tiles_end;

        public:

            inline TileCulling(Renderer& renderer,
                    Tile const* tiles, Tile const* tiles_end, Tile** stack);
 
        protected:

            // flood fill strategy

            inline bool select(Tile* t);
            inline void process(Tile* t);

            void right(Tile*& cursor) const   { cursor += 1; }
            void left(Tile*& cursor)  const   { cursor -= 1; }
            void up(Tile*& cursor)    const   { cursor += yStride(); }
            void down(Tile*& cursor)  const   { cursor -= yStride(); }

            void defer(Tile* t) { *itr_stack++ = t; }
            inline bool deferred(Tile*& cursor);

        private:

            inline unsigned yStride() const;
    };

    class Renderer {

            GpuVertex*      arr_data;
            Tile*           arr_tile;
            GLint*          arr_batch_offs;
            GLsizei*        arr_batch_count;
            GLuint          hnd_vao;
            Tiling          obj_tiling;

            unsigned*       itr_out_index;
            vec3            vec_w_xform;
            float           val_half_persp;
            BrightnessLevel val_min_bright;

        public:

            Renderer(InputVertices const& src,
                     size_t n,
                     unsigned k,
                     BrightnessLevel b,
                     BrightnessLevel bMin) :

                arr_data(0l), 
                arr_tile(0l), 
                obj_tiling(k) {

                this->glAlloc();

                Tiling tiling(k);
                size_t nTiles = tiling.getTileCount();

                arr_data = new GpuVertex[n];
                arr_tile = new Tile[nTiles + 1];
                arr_batch_offs = new GLint[nTiles]; 
                arr_batch_count = new GLsizei[nTiles];

                prepareVertexData(src, n, tiling, b, bMin);

                this->glUpload(n);
            }

            ~Renderer()
            {
                delete[] arr_data;
                delete[] arr_tile;
                delete[] arr_batch_count;
                delete[] arr_batch_offs;

                this->glFree();
            }

            void render(FieldOfView const& fov, BrightnessLevel min_bright)
            {

// fprintf(stderr, "
//      Stars.cpp: rendering at minimal brightness %d\n", min_bright);

                float half_persp = fov.getPerspective() * 0.5f;
                float aspect = fov.getAspectRatio();

                // determine dimensions based on a sought screen diagonal
                //
                //  ww + hh = dd
                //  a = h / w => h = wa
                //  ww + ww aa = dd
                //  ww = dd / (1 + aa)
                float diag = 2.0f * std::sin(half_persp);
                float near = std::cos(half_persp);
                
                float hw = 0.5f * sqrt(diag * diag / (1.0f + aspect * aspect));
                float hh = hw * aspect;

                // cancel all translation
                mat4 matrix = fov.getOrientation();
                matrix[3][0] = 0.0f;
                matrix[3][1] = 0.0f;
                matrix[3][2] = 0.0f;

                // extract local z vector
                vec3 ahead = swizzle<X,Y,Z>( column(matrix, 2) );

                float azimuth = atan2(ahead.x,-ahead.z) + Radians::pi();
                float altitude = atan2(-ahead.y, hypot(ahead.x, ahead.z));
                angleHorizontalPolar<Radians>(azimuth, altitude);
#ifdef FORCE_POSITIVE_ALTITUDE
                altitude = std::max(0.0f, altitude);
#endif
                unsigned tile_index = 
                        obj_tiling.getTileIndex(azimuth, altitude);

// fprintf(stderr, "Stars.cpp: starting on tile #%d\n", tile_index);


#ifdef SEE_LOD 
                mat4 matrix_debug = glm::translate( 
                        glm::frustum(-hw,hw, -hh,hh, near,10.0f), 
                        vec3(0.0f, 0.0f, -4.0f)) * glm::affineInverse(matrix);
#endif

                matrix = glm::frustum(-hw,hw, -hh,hh, near,10.0f)
                        * glm::affineInverse(matrix);

                this->itr_out_index = (unsigned*) arr_batch_offs;
                this->vec_w_xform = swizzle<X,Y,Z>(row(matrix, 3));
                this->val_half_persp = half_persp;
                this->val_min_bright = min_bright;

                floodFill(arr_tile + tile_index, TileCulling(*this, 
                        arr_tile, arr_tile + obj_tiling.getTileCount(),
                        (Tile**) arr_batch_count));

#ifdef SEE_LOD
#define matrix matrix_debug
#endif
                this->glBatch(glm::value_ptr(matrix), prepareBatch(
                        (unsigned*) arr_batch_offs, itr_out_index) );

#ifdef SEE_LOD
#undef matrix
#endif
            }

        private: // renderer construction

            void prepareVertexData(InputVertices const& src,
                                   size_t n, // <-- at bMin and brighter
                                   Tiling const& tiling, 
                                   BrightnessLevel b, 
                                   BrightnessLevel bMin) {

                size_t nTiles = tiling.getTileCount();
                size_t vertex_index = 0u, curr_tile_index = 0u, count_active = 0u;

                arr_tile[0].offset = 0u;
                arr_tile[0].lod = b;
                arr_tile[0].flags = 0u;

                for (InputVertices::const_iterator i = 
                        src.begin(), e = src.end(); i != e; ++i) {

                    BrightnessLevel bv = getBrightness(i->getColor());
                    // filter by alloc brightness
                    if (bv >= bMin) {

                        size_t tile_index = tiling.getTileIndex(*i);
                        assert(tile_index >= curr_tile_index);

                        // moved on to another tile? -> flush
                        if (tile_index != curr_tile_index) {

                            Tile* t = arr_tile + curr_tile_index;
                            Tile* t_last = arr_tile + tile_index;

                            // set count of active vertices (upcoming lod)
                            t->count = count_active;
                            // generate skipped, empty tiles
                            for(size_t offs = vertex_index; ++t != t_last ;) {
                                t->offset = offs, t->count = 0u, 
                                t->lod = b, t->flags = 0u;
                            }

                            // initialize next (as far as possible here)
                            t_last->offset = vertex_index;
                            t_last->lod = b;
                            t_last->flags = 0u;

                            curr_tile_index = tile_index;
                            count_active = 0u;
                        }

                        if (bv >= b)
                            ++count_active;

// fprintf(stderr, "Stars.cpp: Vertex %d on tile #%d\n", vertex_index, tile_index);

                        // write converted vertex
                        arr_data[vertex_index++] = *i;
                    }
                }
                assert(vertex_index == n);
                // flush last tile (see above)
                Tile* t = arr_tile + curr_tile_index; 
                t->count = count_active;
                for (Tile* e = arr_tile + nTiles + 1; ++t != e;) {
                    t->offset = vertex_index, t->count = 0u,
                    t->lod = b, t->flags = 0;
                }
            }

        private: // FOV culling / LOD

            friend class TileCulling;

            bool visitTile(Tile* t) {

                unsigned index =  t - arr_tile;
                *itr_out_index++ = index;

                if (! tileVisible(t, index))
                    return false;

                if (t->lod != val_min_bright)
                    updateVertexCount(t, val_min_bright);

                return true;
            }

            bool tileVisible(Tile* t, unsigned i) {

                float slice = obj_tiling.getSliceAngle();
                unsigned stride = obj_tiling.getAzimuthalTiles();
                float azimuth = (i % stride) * slice;
                float altitude = (i / stride) * slice - Radians::half_pi();
                float gx =  sin(azimuth);
                float gz = -cos(azimuth);
                float exz = cos(altitude);
                vec3 tile_center = vec3(gx * exz, sin(altitude), gz * exz);
                float w = dot(vec_w_xform, tile_center);

                float half_slice = 0.5f * slice;
                float daz = half_slice * cos(abs(altitude) - half_slice);
                float dal = half_slice;
                float near = cos(val_half_persp + sqrt(daz*daz+dal*dal));

// fprintf(stderr, "Stars.cpp: checking tile #%d, w = %f, near = %f\n", i,  w, near);

                return w > near;
            }

            void updateVertexCount(Tile* t, BrightnessLevel minBright) {

                // a growing number of stars needs to be rendereed when the 
                // minimum brightness decreases
                // perform a binary search in the so found partition for the
                // new vertex count of this tile

                GpuVertex const* start = arr_data + t[0].offset;
                GpuVertex const* end = arr_data + t[1].offset;

                assert(end >= start);

                if (start == end)
                    return;

                if (t->lod < minBright)
                    end = start + t->count;
                else
                    start += (t->count > 0 ? t->count - 1 : 0);

                end = std::upper_bound(
                        start, end, minBright, GreaterBrightness());

                assert(end >= arr_data + t[0].offset);

                t->count = end - arr_data - t[0].offset; 
                t->lod = minBright;
            }

            unsigned prepareBatch(unsigned const* indices, 
                                  unsigned const* indicesEnd) {

                unsigned nRanges = 0u;
                GLint* offs = arr_batch_offs;
                GLsizei* count = arr_batch_count;

                for (unsigned* i = (unsigned*) arr_batch_offs; 
                        i != indicesEnd; ++i) {

                    Tile* t = arr_tile + *i;
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
#define glBindVertexArray glBindVertexArrayAPPLE 
#define glGenVertexArrays glGenVertexArraysAPPLE 
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE 
#endif
            void glAlloc() {

                glGenVertexArrays(1, & hnd_vao);
            }

            void glFree() {

                glDeleteVertexArrays(1, & hnd_vao);
            }

            void glUpload(GLsizei n) {

                GLuint vbo;
                glGenBuffers(1, & vbo);

                glBindVertexArray(hnd_vao);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER,
                        n * sizeof(GpuVertex), arr_data, GL_STATIC_DRAW);
                glInterleavedArrays(GL_C4UB_V3F, sizeof(GpuVertex), 0l);

                glBindVertexArray(0); 
            }

            void glBatch(GLfloat const* matrix, GLsizei n_ranges) {
// fprintf(stderr, "Stars.cpp: rendering %d-multibatch\n", n_ranges);

// for (int i = 0; i < n_ranges; ++i)
//     fprintf(stderr, "Stars.cpp: Batch #%d - %d stars @ %d\n", i, 
//             arr_batch_offs[i], arr_batch_count[i]);

                // setup modelview matrix (identity)
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();

                // set projection matrix
                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glLoadMatrixf(matrix);

                // render
                glBindVertexArray(hnd_vao);

                glEnable(GL_POINT_SMOOTH);
                glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
                glPointSize(1.42f);

                glMultiDrawArrays(GL_POINTS,
                        arr_batch_offs, arr_batch_count, n_ranges);

                // restore state
                glBindVertexArray(0); 
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                glPopMatrix();
            }
#ifdef __APPLE__
#undef glBindVertexArray 
#undef glGenVertexArrays 
#undef glDeleteVertexArrays
#endif
    };

    TileCulling::TileCulling(Renderer& renderer,
                             Tile const* tiles,
                             Tile const* tiles_end,
                             Tile** stack)
    :
        ref_renderer(renderer),
        arr_stack(stack),
        itr_stack(stack), 
        arr_tile(tiles), 
        itr_tiles_end(tiles_end) {
    }

    bool TileCulling::select(Tile* t) {

        if (t < arr_tile || t >= itr_tiles_end ||
                !! (t->flags & Tile::visited)) {

            return false;
        }
        if (! (t->flags & Tile::checked)) {

            if (ref_renderer.visitTile(t))
                t->flags |= Tile::render;
        }
        return !! (t->flags & Tile::render);
    }

    void TileCulling::process(Tile* t) { 

        t->flags |= Tile::visited;
    }

    bool TileCulling::deferred(Tile*& cursor) {

        if (itr_stack != arr_stack) {
            cursor = *--itr_stack;
            return true;
        }
        return false;
    }

    unsigned TileCulling::yStride() const {

        return ref_renderer.obj_tiling.getAzimuthalTiles();
    }
}


 
class Stars::body
{
    InputVertices       seq_input;
    unsigned            val_tile_resolution;

    double              val_lod_fraction;
    double              val_lod_low_water_mark;
    double              val_lod_high_water_mark;
    double              val_lod_overalloc;
    size_t              val_lod_n_alloc;
    size_t              val_lod_n_render;
    BrightnessLevels    seq_lod_brightness;
    BrightnessLevel     val_lod_brightness;
    BrightnessLevel     val_lod_alloc_brightness;

    Renderer*           ptr_renderer;

public:

    body() :
        val_tile_resolution(20), 
        val_lod_fraction(1.0),
        val_lod_low_water_mark(0.8),
        val_lod_high_water_mark(1.0),
        val_lod_overalloc(1.2),
        val_lod_n_alloc(0),
        val_lod_n_render(0),
        val_lod_brightness(0),
        val_lod_alloc_brightness(0),
        ptr_renderer(0l) {
    }

    bool readInput(const char* url, unsigned limit)
    {
        InputVertices vertices;

        if (! Loader().loadVertices(vertices, url, limit))
            return false;

        BrightnessLevels brightness;
        extractBrightnessLevels(brightness, vertices);

        assert(brightness.size() == vertices.size());

        for (BrightnessLevels::iterator i = brightness.begin(); i != brightness.end() - 1; ++i)
        {
            BrightnessLevels::iterator next = i + 1;
            if (next != brightness.end())
                assert( *i >= *next );
        }

        // input is read, now run the entire data pipeline on the new input

        {
            // TODO input mutex

            seq_input.swap(vertices);

            unsigned k = val_tile_resolution;

            size_t n, nRender;
            BrightnessLevel bMin, b;
            double rcpChange;

            // we'll have to build a new LOD state for a new total N,
            // ideally keeping allocation size and number of vertices

            {   // TODO lod mutex

                size_t newLast = seq_input.size() - 1;

                // reciprocal change N_old/N_new tells us how to scale
                // the fractions
                rcpChange = min(1.0, double(vertices.size()) / seq_input.size());

                // initialization? use defaults / previously set values
                if (rcpChange == 0.0) {

                    rcpChange = 1.0;

                    nRender = size_t(round(val_lod_fraction * newLast));
                    n = min(newLast, size_t(round(val_lod_overalloc * nRender)));

                } else {

                    // cannot allocate or render more than we have
                    n = min(newLast, val_lod_n_alloc);
                    nRender = min(newLast, val_lod_n_render);
                }

                // determine new minimum brightness levels
                bMin = brightness[n];
                b = brightness[nRender];

                // adjust n
                n = std::upper_bound(
                        brightness.begin() + n - 1,
                        brightness.end(), 
                        bMin, GreaterBrightness() ) - brightness.begin();
            }

            // invoke next stage
            try {

                this->retile(n, k, b, bMin);

            } catch (...) {

                // rollback transaction and rethrow
                vertices.swap(seq_input);
                throw;
            }

            // finally publish the new LOD state

            {   // TODO lod mutex

                seq_lod_brightness.swap(brightness);
                val_lod_fraction *= rcpChange;
                val_lod_low_water_mark *= rcpChange;
                val_lod_high_water_mark *= rcpChange;
                val_lod_overalloc *= rcpChange;
                val_lod_n_alloc = n;
                val_lod_n_render = nRender;
                val_lod_alloc_brightness = bMin;
                // keep last, it's accessed asynchronously
                val_lod_brightness = b;
            }
        }

        return true;
    }

    bool setResolution(unsigned k) {

        if (k <= 3) {
            return false;
        }

// fprintf(stderr, "Stars.cpp: setResolution(%d)\n", k);

        if (k != val_tile_resolution) { // TODO make atomic

            // TODO input mutex

            unsigned n;
            BrightnessLevel b, bMin;

            {   // TODO lod mutex

                n = val_lod_n_alloc;
                b = val_lod_brightness;
                bMin = val_lod_alloc_brightness;
            }

            this->retile(n, k, b, bMin);

            return true;
        } else {
            return false;
        }
    } 



    void retile(size_t n, unsigned k, 
                BrightnessLevel b, BrightnessLevel bMin) {

        Tiling tiling(k);
        TileSortScanner scanner(tiling);
        radix2InplaceSort(seq_input.begin(), seq_input.end(), scanner);

// fprintf(stderr, 
//        "Stars.cpp: recreateRenderer(%d, %d, %d, %d)\n", n, k, b, bMin);
 
        recreateRenderer(n, k, b, bMin);

        val_tile_resolution = k;
    }

    double changeLOD(double factor, double overalloc, double realloc) {

        assert(overalloc >= realloc && realloc >= 0.0);
        assert(overalloc <= 1.0 && realloc <= 1.0);

//  fprintf(stderr, 
//        "Stars.cpp: changeLOD(%lf, %lf, %lf)\n", factor, overalloc, realloc);

        size_t n, nRender;
        BrightnessLevel bMin, b;
        double fraction, lwm, hwm;

        {   // TODO lod mutex
    
            // acuire a consistent copy of the current LOD state
            fraction = val_lod_fraction;
            lwm = val_lod_low_water_mark;
            hwm = val_lod_high_water_mark;
            size_t last = seq_lod_brightness.size() - 1;

            // apply factor
            fraction = max(0.0, min(1.0, fraction * factor));

            // calculate allocation size and corresponding brightness
            // threshold
            double oaFract = std::min(fraction * (1.0 + overalloc), 1.0);
            n = size_t(round(oaFract * last));
            bMin = seq_lod_brightness[n];
            n = std::upper_bound(
                    seq_lod_brightness.begin() + n - 1,
                    seq_lod_brightness.end(), 
                    bMin, GreaterBrightness() ) - seq_lod_brightness.begin();

            // also determine number of vertices to render and brightness
            nRender = size_t(round(fraction * last));
            // Note: nRender does not have to be accurate
            b = seq_lod_brightness[nRender];
            // this setting controls the renderer, also keep b as the 
            // brightness becomes volatile as soon as the mutex is
            // released
            val_lod_brightness = b; // TODO make atomic

// fprintf(stderr, "Stars.cpp: "
//        "fraction = %lf, oaFract = %lf, n = %d, n' = %d, bMin = %d, b = %d\n", 
//        fraction, oaFract, size_t(round(oaFract * last)), n, bMin, b);

            // will not have to reallocate? set new fraction right away
            // (it is consistent with the rest of the state in this case)
            if (fraction >= val_lod_low_water_mark 
                    && fraction <= val_lod_high_water_mark) {

                val_lod_fraction = fraction;
                return fraction;
            }
        }

        // reallocate
        {   // TODO input mutex
            recreateRenderer(n, val_tile_resolution, b, bMin); 

 fprintf(stderr, "Stars.cpp: LOD reallocation\n"); 
 
            // publish new lod state
            {   // TODO lod mutex
                val_lod_n_alloc = n;
                val_lod_n_render = nRender;

                val_lod_fraction = fraction;
                val_lod_low_water_mark = fraction * (1.0 - realloc);
                val_lod_high_water_mark = fraction * (1.0 + realloc);
                val_lod_overalloc = fraction * (1.0 + overalloc);
                val_lod_alloc_brightness = bMin;
            }
        }
        return fraction;
    }

    void recreateRenderer(size_t n, unsigned k, 
                          BrightnessLevel b, BrightnessLevel bMin) {

        Renderer* renderer = new Renderer(seq_input, n, k, b, bMin);
        swap(ptr_renderer, renderer); // TODO make atomic
        delete renderer; // will be NULL when was in use
    }

    void render(FieldOfView const& fov) {

        // check out renderer
        Renderer* renderer = 0l;
        swap(ptr_renderer, renderer); // TODO make atomic

        // have it render
        if (renderer != 0l) {

            BrightnessLevel b = val_lod_brightness; // make atomic

            renderer->render(fov, b);
        }

        // check in - or dispose if there is a new one
        // TODO make atomic (CAS)
        if (! ptr_renderer) {
            ptr_renderer = renderer; 
        } else {
            delete renderer;
        }
    }
};

Stars::Stars() : 
    ptr_body(0l) { 
    ptr_body = new body; 
}
Stars::~Stars() { 
    delete ptr_body; 
}

bool Stars::readInput(const char* url, unsigned limit) {
    return ptr_body->readInput(url, limit); 
}

bool Stars::setResolution(unsigned k) { 
    return ptr_body->setResolution(k); 
}

float Stars::changeLOD(float fraction, float overalloc, float realloc) { 
    return float(ptr_body->changeLOD(fraction, overalloc, realloc));
} 

void Stars::render(FieldOfView const& fov) {
    ptr_body->render(fov); 
}



