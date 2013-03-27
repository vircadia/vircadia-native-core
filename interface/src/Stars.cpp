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

#define DEG2RAD 0.017453292519f

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

namespace
{
    using std::swap;

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

    class InputVertex
    {
            unsigned    val_color;
            float       val_azimuth;
            float       val_altitude;
        public:

            InputVertex(float azimuth, float altitude, unsigned color)
            {
                val_color = color >> 16 & 0xffu | color & 0xff00u |
                        color << 16 & 0xff0000u | 0xff000000u;

                angleHorizontalPolar<Degrees>(azimuth, altitude);

                val_azimuth = azimuth;
                val_altitude = altitude;
            }

            float getAzimuth() const { return val_azimuth; }
            float getAltitude() const { return val_altitude; }
            unsigned getColor() const { return val_color; } 
    }; 

    typedef std::vector<InputVertex> InputVertices;

    typedef uint16_t BrightnessLevel;
    typedef std::vector<BrightnessLevel> BrightnessLevels;
    const unsigned BrightnessBits = 16u;

    BrightnessLevel getBrightness(unsigned c)
    {
        unsigned r = (c >> 16) & 0xff;
        unsigned g = (c >> 8) & 0xff;
        unsigned b = c & 0xff;
        return BrightnessLevel((r*r+g*g+b*b) >> 1);
    }

    struct BrightnessSortScanner : Radix2IntegerScanner<BrightnessLevel>
    {
        typedef Radix2IntegerScanner<BrightnessLevel> Base;
        BrightnessSortScanner() : Base(BrightnessBits) { }
        bool bit(BrightnessLevel const& k, state_type& s)
        { return ! Base::bit(k,s); }
    };

    void extractBrightnessLevels(BrightnessLevels& dst, InputVertices const& src)
    {
        dst.clear();
        dst.reserve(src.size());
        for (InputVertices::const_iterator i = 
                src.begin(), e = src.end(); i != e; ++i)
            dst.push_back( getBrightness(i->getColor()) );

        radix2InplaceSort(dst.begin(), dst.end(), BrightnessSortScanner());
    }

    template< class Unit >
    class HorizontalTiling
    {
            unsigned    val_k;
            float       val_rcp_slice;
            unsigned    val_bits;
        public:

            HorizontalTiling(unsigned k)
                : val_k(k), val_rcp_slice(k / Unit::twice_pi())
            {
                val_bits = ceil(log2(getTileCount() ));
            }

            unsigned getAzimuthalTiles() const { return val_k; }
            unsigned getAltitudinalTiles() const { return val_k / 2 + 1; }
            unsigned getTileIndexBits() const { return val_bits; }

            unsigned getTileCount() const
            {
                return getAzimuthalTiles() * getAltitudinalTiles();
            }

            unsigned getTileIndex(float azimuth, float altitude) const
            {
                return discreteAngle(azimuth) % val_k +
                    discreteAngle(altitude + Unit::half_pi()) * val_k;
            }

            unsigned getTileIndex(InputVertex const& v) const
            {
                return getTileIndex(v.getAzimuth(), v.getAltitude());
            }

            float getSliceAngle() const
            {
                return 1.0f / val_rcp_slice;
            }

            float getReciprocalSliceAngle() const
            {
                return val_rcp_slice;
            }

        private:

            unsigned discreteAngle(float unsigned_angle) const
            {
                return unsigned(round(unsigned_angle * val_rcp_slice));
            }

    };

    class TileSortScanner : public Radix2IntegerScanner<unsigned>
    {
            HorizontalTiling<Degrees> obj_tiling;

            typedef Radix2IntegerScanner<unsigned> Base;
        public:

            explicit TileSortScanner(HorizontalTiling<Degrees> const& tiling)
                : Base(tiling.getTileIndexBits() + BrightnessBits),
                obj_tiling(tiling)
            { }

            bool bit(InputVertex const& v, state_type const& s) const
            {
                // inspect (tile_index, brightness) tuples
                unsigned key = getBrightness(v.getColor());
                key |= obj_tiling.getTileIndex(v) << BrightnessBits;
                return Base::bit(key, s); 
            }
    };

    struct Tile
    {
        uint16_t        offset;
        uint16_t        count; 
        BrightnessLevel lod;
        uint16_t        flags;

        static uint16_t const checked = 1;
        static uint16_t const visited = 2;
        static uint16_t const render = 4;
    };


    class GpuVertex
    {
            unsigned    val_color;
            float       val_x;
            float       val_y;
            float       val_z;
        public:

            GpuVertex() { }

            GpuVertex(InputVertex const& in)
            {
                val_color = in.getColor();
                float azimuth = in.getAzimuth() * DEG2RAD;
                float altitude = in.getAltitude() * DEG2RAD;

                // ground vector in x/z plane...
                float gx =  sin(azimuth);
                float gz = -cos(azimuth);

                // ...elevated in y direction by altitude
                float exz = cos(altitude);
                val_x = gx * exz;
                val_y = sin(altitude);
                val_z = gz * exz;
            }

            unsigned getColor() const { return val_color; } 
    };

    struct GreaterBrightness
    {
        bool operator()(InputVertex const& lhs, InputVertex const& rhs) const
        {
            return getBrightness(lhs.getColor())
                    > getBrightness(rhs.getColor());
        }
        bool operator()(BrightnessLevel lhs, GpuVertex const& rhs) const
        {
            return lhs > getBrightness(rhs.getColor());;
        }
    };

    class Loader : UrlReader
    {
            InputVertices*  arr_vertex;
            unsigned        val_limit;

            unsigned        val_lineno;
            char const*     str_actual_url;

            unsigned        val_records_read;
            BrightnessLevel val_min_brightness;
        public:

            bool loadVertices(
                    InputVertices& destination, char const* url, unsigned limit)
            {
                arr_vertex = & destination;
                val_limit = limit;
                str_actual_url = url; // in case we fail early

                if (! UrlReader::readUrl(url, *this))
                {
                    fprintf(stderr, "%s:%d: %s\n",
                            str_actual_url, val_lineno, getError());

                    return false;
                }
                return true;
            }

        protected:

            friend class UrlReader;

            void begin(char const* url,
                    char const* type, int64_t size, int64_t stardate)
            {
                val_lineno = 0u;
                str_actual_url = url; // new value in http redirect

                arr_vertex->clear();
                arr_vertex->reserve(val_limit);
            }
            
            size_t transfer(char* input, size_t bytes)
            {
                size_t consumed = 0u;
                char const* end = input + bytes;
                char* line, * next = input;

                for (;;)
                {
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
                    if (sscanf(line, " %f %f #%x", & azi, & alt, & c) == 3)
                    {
                        if (val_limit > 0)
                        {
                            if (val_records_read++ == val_limit)
                            {
                                std::make_heap(
                                    arr_vertex->begin(), arr_vertex->end(),
                                    GreaterBrightness() );

                                val_min_brightness = getBrightness(
                                        arr_vertex->begin()->getColor() );
                            }
                            if (arr_vertex->size() == val_limit)
                            {
                                if (val_min_brightness >= getBrightness(c))
                                    continue;

                                std::pop_heap(
                                    arr_vertex->begin(), arr_vertex->end(),
                                    GreaterBrightness() );
                                arr_vertex->pop_back();
                            }
                        }

                        arr_vertex->push_back( InputVertex(azi, alt, c) );

                        if (val_limit > 0 && val_records_read > val_limit)
                        {
                            std::push_heap(
                                arr_vertex->begin(), arr_vertex->end(),
                                GreaterBrightness() );
                            arr_vertex->pop_back();

                            val_min_brightness = getBrightness(
                                    arr_vertex->begin()->getColor() );
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Stars.cpp:%d: Bad input from %s\n", 
                                val_lineno, str_actual_url);
                    }

                }
                return consumed;
            }

            void end(bool ok)
            { }
    };

    class Renderer;

    class TileCulling
    {
            Renderer&           ref_renderer;
            Tile**              ptr_stack;
            Tile const* const   arr_tile;
            Tile const* const   ptr_tiles_end;

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

            void defer(Tile* t) { *ptr_stack++ = t; }
            inline bool deferred(Tile*& cursor);

        private:

            inline unsigned yStride() const;
    };

    class Renderer
    {
            typedef HorizontalTiling<Radians> Tiling;

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

            Renderer(InputVertices const& src, size_t n,
                    unsigned k, BrightnessLevel b, BrightnessLevel b_max)
                : arr_data(0l), arr_tile(0l), obj_tiling(k)
            {
                this->glAlloc();

                HorizontalTiling<Degrees> tiling(k);
                size_t n_tiles = tiling.getTileCount();

                arr_data = new GpuVertex[n];
                arr_tile = new Tile[n_tiles + 1];
                arr_batch_offs = new GLint[n_tiles]; 
                arr_batch_count = new GLsizei[n_tiles];

                prepareVertexData(src, n, tiling, b, b_max);

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
                unsigned tile_index = 
                        obj_tiling.getTileIndex(azimuth, altitude);

// fprintf(stderr, "Stars.cpp: starting on tile #%d\n", tile_index);

#ifdef SEE_LOD // define to peek behind the scenes
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
#define matrix debug_matrix
#endif
                this->glBatch(glm::value_ptr(matrix), prepareBatch(
                        (unsigned*) arr_batch_offs, itr_out_index) );

#ifdef SEE_LOD
#undef matrix
#endif
            }

        private: // renderer construction

            void prepareVertexData(InputVertices const& src, size_t n, 
                    HorizontalTiling<Degrees> const& tiling, BrightnessLevel b, 
                    BrightnessLevel b_max)
            {
                size_t n_tiles = tiling.getTileCount();
                size_t vertex_index = 0u, curr_tile_index = 0u, count_active = 0u;

                arr_tile[0].offset = 0u;
                arr_tile[0].lod = b;
                arr_tile[0].flags = 0u;

                for (InputVertices::const_iterator i = 
                        src.begin(), e = src.end(); i != e; ++i)
                {
                    BrightnessLevel bv = getBrightness(i->getColor());
                    // filter by alloc brightness
                    if (bv >= b_max)
                    {
                        size_t tile_index = tiling.getTileIndex(*i);
                        assert(tile_index >= curr_tile_index);

                        // moved on to another tile? -> flush
                        if (tile_index != curr_tile_index)
                        {
                            Tile* t = arr_tile + curr_tile_index;
                            Tile* t_last = arr_tile + tile_index;

                            // set count of active vertices (upcoming lod)
                            t->count = count_active;
                            // generate skipped, empty tiles
                            for(size_t offs = t_last->offset; ++t != t_last ;)
                                t->offset = offs, t->count = 0u, 
                                t->lod = b, t->flags = 0u;

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
                for (Tile* e = arr_tile + n_tiles + 1; ++t != e ;)
                    t->offset = vertex_index, t->count = 0u,
                    t->lod = b, t->flags = 0;
            }

        private: // FOV culling / LOD

            friend class TileCulling;

            bool visitTile(Tile* t)
            {
                unsigned index =  t - arr_tile;
                *itr_out_index++ = index;

                if (! tileVisible(t, index))
                    return false;

                if (t->lod != val_min_bright)
                    updateVertexCount(t, val_min_bright);

                return true;
            }

            bool tileVisible(Tile* t, unsigned i)
            {
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

            void updateVertexCount(Tile* t, BrightnessLevel min_bright)
            {
                // a growing number of stars needs to be rendereed when the 
                // minimum brightness decreases
                // perform a binary search in the so found partition for the
                // new vertex count of this tile

                GpuVertex const* start = arr_data + t[0].offset;
                GpuVertex const* end = arr_data + t[1].offset;

                if (start == end)
                    return;

                if (t->lod < min_bright)
                    end = start + t->count;
                else
                    start += (t->count > 0 ? t->count - 1 : 0);

                end = std::upper_bound(
                        start, end, min_bright, GreaterBrightness());

                t->count = end - arr_data + t[0].offset;
                t->lod = min_bright;
            }

            unsigned prepareBatch(unsigned* indices, unsigned* indices_end) 
            {
                unsigned n_ranges = 0u;
                GLint* offs = arr_batch_offs;
                GLsizei* count = arr_batch_count;
                for (unsigned* i = (unsigned*) arr_batch_offs; 
                        i != indices_end; ++i)
                {
                    Tile* t = arr_tile + *i;
                    if ((t->flags & Tile::render) > 0u && t->count > 0u)
                    {
                        *offs++ = t->offset;
                        *count++ = t->count;
                        ++n_ranges;
                    }
                    t->flags = 0;
                }
                return n_ranges;
            }

        private: // gl API handling 

            void glAlloc()
            {
                glGenVertexArrays(1, & hnd_vao);
            }

            void glFree()
            {
                glDeleteVertexArrays(1, & hnd_vao);
            }

            void glUpload(GLsizei n)
            {
                GLuint vbo;
                glGenBuffers(1, & vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER,
                        n * sizeof(GpuVertex), arr_data, GL_STATIC_DRAW);

                glBindVertexArray(hnd_vao);
                glInterleavedArrays(GL_C4UB_V3F, sizeof(GpuVertex), 0l);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);

                glBindVertexArray(0); 
            }

            void glBatch(GLfloat const* matrix, GLsizei n_ranges)
            {
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
                glPointSize(1.0);
                glMultiDrawArrays(GL_POINTS,
                        arr_batch_offs, arr_batch_count, n_ranges);

                // restore state
                glBindVertexArray(0); 
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                glPopMatrix();
            }
    };

    TileCulling::TileCulling(
            Renderer& renderer,
            Tile const* tiles, Tile const* tiles_end, Tile** stack)

        :   ref_renderer(renderer),
            ptr_stack(stack), arr_tile(tiles), ptr_tiles_end(tiles_end)
    { }

    bool TileCulling::select(Tile* t)
    {
        if (t < arr_tile || t >= ptr_tiles_end ||
                !! (t->flags & Tile::visited))
            return false;

        if (! (t->flags & Tile::checked))
        {
            if (ref_renderer.visitTile(t))
                t->flags |= Tile::render;
        }
        return !! (t->flags & Tile::render);
    }

    void TileCulling::process(Tile* t) { t->flags |= Tile::visited; }

    bool TileCulling::deferred(Tile*& cursor)
    {
        if (ptr_stack != (Tile**) ref_renderer.arr_batch_count)
        {
            cursor = *--ptr_stack;
            return true;
        }
        return false;
    }

    unsigned TileCulling::yStride() const
    {
        return ref_renderer.obj_tiling.getAzimuthalTiles();
    }
}
    
struct Stars::body
{
    InputVertices   vec_input;
    unsigned        val_tile_resolution;

    BrightnessLevels    vec_lod_brightness;
    BrightnessLevel     val_lod_brightness;
    BrightnessLevel     val_lod_max_brightness;
    float               val_lod_current_alloc;
    float               val_lod_low_water_mark;
    float               val_lod_high_water_mark;

    Renderer*           ptr_renderer;


    body()
        : val_tile_resolution(16), val_lod_brightness(0),
            val_lod_max_brightness(0), val_lod_current_alloc(1.0f),
            val_lod_low_water_mark(0.99f), val_lod_high_water_mark(1.0f),
            ptr_renderer(0l)
    { }

    bool readInput(const char* url, unsigned limit)
    {
        InputVertices new_vertices;

        if (! Loader().loadVertices(new_vertices, url, limit))
            return false;

        BrightnessLevels new_brightness;
        extractBrightnessLevels(new_brightness, new_vertices);

        {
            // TODO input mutex

            vec_input.swap(new_vertices);

            try
            {
                retile(val_tile_resolution);
            }
            catch (...)
            {
                // rollback transaction
                new_vertices.swap(vec_input);
                throw;
            }

            {
                // TODO lod mutex
                vec_lod_brightness.swap(new_brightness);
            }
        }
        new_vertices.clear();
        new_brightness.clear();

        return true;
    }

    void setResolution(unsigned k)
    {
        if (k != val_tile_resolution)
        {
            // TODO input mutex
            retile(k);
        }
    } 

    void retile(unsigned k)
    {
        HorizontalTiling<Degrees> tiling(k);
        TileSortScanner scanner(tiling);
        radix2InplaceSort(vec_input.begin(), vec_input.end(), scanner);

        recreateRenderer(vec_input.size(), k, 
                val_lod_brightness, val_lod_max_brightness);

        val_tile_resolution = k;
    }

    void setLOD(float fraction, float overalloc, float realloc)
    {
        assert(fraction >= 0.0f && fraction <= 0.0f);
        assert(overalloc >= realloc && realloc >= 0.0f);
        assert(overalloc <= 1.0f && realloc <= 1.0f);

        float lwm, hwm;
        float oa_fraction = std::min(fraction * (1.0f + oa_fraction), 1.0f);
        size_t oa_new_size;
        BrightnessLevel b, b_max;
        {
            // TODO lod mutex
            // Or... There is just one write access, here - so LOD state
            // could be CMPed as well...
            lwm = val_lod_low_water_mark;
            hwm = val_lod_high_water_mark;
            size_t last = vec_lod_brightness.size() - 1;
            val_lod_brightness = b = 
                    vec_lod_brightness[ size_t(fraction * last) ];
            oa_new_size = size_t(oa_fraction * last);
            b_max = vec_lod_brightness[oa_new_size++];
        }

        // have to reallocate?
        if (fraction < lwm || fraction > hwm)
        {
            // TODO input mutex
            recreateRenderer(oa_new_size, val_tile_resolution, b, b_max); 

            {
                // TODO lod mutex
                val_lod_current_alloc = fraction;
                val_lod_low_water_mark = fraction * (1.0f - realloc);
                val_lod_high_water_mark = fraction * (1.0f + realloc);
                val_lod_max_brightness = b_max;
            }
        }
    }

    void recreateRenderer(
            size_t n, unsigned k, BrightnessLevel b, BrightnessLevel b_max)
    {
        Renderer* renderer = new Renderer(vec_input, n, k, b, b_max);
        swap(ptr_renderer, renderer); // TODO make atomic
        delete renderer; // will be NULL when was in use
    }

    void render(FieldOfView const& fov)
    {
        // check out renderer
        Renderer* renderer = 0l;
        swap(ptr_renderer, renderer); // TODO make atomic
        float new_brightness = val_lod_brightness; // make atomic

        // have it render
        if (renderer)
            renderer->render(fov, new_brightness);

        // check in - or dispose if there is a new one
        // TODO make atomic (CAS)
        if (! ptr_renderer)
            ptr_renderer = renderer; 
        else delete renderer;
    }
};

Stars::Stars() : ptr_body(0l) { ptr_body = new body; }
Stars::~Stars() { delete ptr_body; }

bool Stars::readInput(const char* url, unsigned limit) 
{ return ptr_body->readInput(url, limit); }

void Stars::setResolution(unsigned k)
{ ptr_body->setResolution(k); }

void Stars::setLOD(float fraction, float overalloc, float realloc)
{ ptr_body->setLOD(fraction, 0.0f, 0.0f); } // TODO enable once implemented

void Stars::render(FieldOfView const& fov) 
{ ptr_body->render(fov); }



