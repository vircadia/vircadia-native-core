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

#include <stddef.h>
#include <stdint.h>
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

#define DEG2RAD 0.017453292519f

/* Data pipeline
 * -------------
 *
 * ->> readInput -(load)--+---- (get brightness & sort) ---> brightness LUT
 *                        |              |        
 * ->> setResolution --+  |             >extractBrightnessLevels<
 *                     V  |
 *            (sort by (tile,brightness))
 *                        |         |
 * ->> setLOD  ---+       |    >retile<   ->> setLOD --> (just parameterizes
 *                V       V                             renderer when on-GPU
 *          (filter by max-LOD brightness,              data suffices)
 *            build tile info for rendering)
 *                        |             |
 *                        V         >recreateRenderer<
 *                 (set new renderer)/
 *
 *
 * (process), ->> entry point, ---> data flow, >internal routine<
 *
 *
 * Open issues
 * -----------
 *
 * o FOV culling is too eager - gotta revisit
 * o atomics/mutexes need to be added as annotated in the source to allow
 *   concurrent threads to pull the strings to e.g. have a low priority
 *   thread run the data pipeline for update -- rendering is wait-free
 */

namespace
{
    using std::swap;
    using std::min;
    using std::max;

    using glm::mat4;
    using glm::value_ptr;

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
                unsigned result;
                return discreteAngle(azimuth) % val_k +
                    discreteAngle(altitude + Unit::half_pi()) * val_k;
            }

            unsigned getTileIndex(InputVertex const& v) const
            {
                return getTileIndex(v.getAzimuth(), v.getAltitude());
            }

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
        uint16_t offset;
        uint16_t count; // according to previous lod setting
    };


    struct GpuVertex
    {
            unsigned    val_color;
            float       val_x;
            float       val_y;
            float       val_z;
        //

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

//fprintf(stderr, "Stars.cpp: GpuVertex created (%x,%f,%f,%f)\n", val_color, val_x, val_y, val_z);
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

                ptr_vertices->clear();
                ptr_vertices->reserve(val_limit);
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
                        if (val_records_read++ == val_limit)
                        {
                            std::make_heap(
                                ptr_vertices->begin(), ptr_vertices->end(),
                                GreaterBrightness() );

                            val_min_brightness = getBrightness(
                                    ptr_vertices->begin()->getColor() );
                        }
                        if (ptr_vertices->size() == val_limit)
                        {
                            if (val_min_brightness >= getBrightness(c))
                                continue;

                            std::pop_heap(
                                ptr_vertices->begin(), ptr_vertices->end(),
                                GreaterBrightness() );
                            ptr_vertices->pop_back();
                        }

                        ptr_vertices->push_back( InputVertex(azi, alt, c) );

                        if (val_records_read > val_limit)
                        {
                            std::push_heap(
                                ptr_vertices->begin(), ptr_vertices->end(),
                                GreaterBrightness() );
                            ptr_vertices->pop_back();

                            val_min_brightness = getBrightness(
                                    ptr_vertices->begin()->getColor() );
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
            {
            }
    };

    class Renderer
    {
            GpuVertex*      ptr_data;
            Tile*           ptr_tiles;
            BrightnessLevel val_lod;
            unsigned        val_tile_resolution;
            GLint*          ptr_batch_offs;
            GLsizei*        ptr_batch_count;
            GLuint          hnd_vao;
        public:

            Renderer(InputVertices const& src, size_t n,
                    unsigned k, BrightnessLevel b, BrightnessLevel b_max)
                : ptr_data(0l), ptr_tiles(0l), 
                    val_lod(b), val_tile_resolution(k)
            {

                HorizontalTiling<Degrees> tiling(k);
                size_t n_tiles = tiling.getTileCount();

                ptr_data = new GpuVertex[n];
                ptr_tiles = new Tile[n_tiles + 1];
                // TODO tighten bounds and save some memory
                ptr_batch_offs = new GLint[n_tiles]; 
                ptr_batch_count = new GLsizei[n_tiles];

                size_t vertex_index = 0u, curr_tile_index = 0u, count_active = 0u;

                for (InputVertices::const_iterator i = 
                        src.begin(), e = src.end(); i != e; ++i)
                {
                    BrightnessLevel bv = getBrightness(i->getColor());
                    // filter by alloc brightness
                    if (bv >= b_max)
                    {
                        size_t tile_index = tiling.getTileIndex(*i);

                        assert(tile_index >= curr_tile_index);

                        // moved to another tile?
                        if (tile_index != curr_tile_index)
                        {
                            Tile* t = ptr_tiles + curr_tile_index;
                            Tile* t_last = ptr_tiles + tile_index;

                            // set count of active vertices (upcoming lod)
                            t->count = count_active;

                            // generate skipped entries
                            for(size_t offs = t_last->offset; ++t != t_last ;)
                                t->offset = offs, t->count = 0u;

                            // set offset of the beginning tile`
                            t_last->offset = vertex_index;

                            curr_tile_index = tile_index;
                            count_active = 0u;
                        }

                        if (bv >= b)
                            ++count_active;

//fprintf(stderr, "Stars.cpp: Vertex %d on tile #%d\n", vertex_index, tile_index);

                        // write converted vertex
                        ptr_data[vertex_index++] = *i;
                    }
                }
                assert(vertex_index == n);
                // finish last tile (see above)
                Tile* t = ptr_tiles + curr_tile_index; 
                t->count = count_active;
                for (Tile* e = ptr_tiles + n_tiles + 1; ++t != e ;)
                    t->offset = vertex_index, t->count = 0u;

                // OpenGL upload

                GLuint vbo;
                glGenBuffers(1, & vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER,
                        n * sizeof(GpuVertex), ptr_data, GL_STATIC_DRAW);

                glGenVertexArrays(1, & hnd_vao);
                glBindVertexArray(hnd_vao);
                glInterleavedArrays(GL_C4UB_V3F, sizeof(GpuVertex), 0l);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);

                glBindVertexArray(0); 

            }

            ~Renderer()
            {
                delete[] ptr_data;
                delete[] ptr_tiles;
                delete[] ptr_batch_count;
                delete[] ptr_batch_offs;
                glDeleteVertexArrays(1, & hnd_vao);
            }

            void render(FieldOfView fov, BrightnessLevel lod)
            {
                mat4 local_space = fov.getOrientation();
                HorizontalTiling<Radians> tiling(val_tile_resolution);

                // get z direction
                float x = local_space[2][0];
                float y = local_space[2][1];
                float z = local_space[2][2];

                // to polar
                float azimuth = atan2(x,-z) + Radians::pi();
                float altitude = atan2(y, sqrt(x*x+z*z));

fprintf(stderr, "Stars.cpp: viewer azimuth = %f, altitude = %f\n", azimuth, altitude);

                // half diagonal perspective angle
                float hd_pers = fov.getPerspective() * 0.5f;

                unsigned azi_dim = tiling.getAzimuthalTiles();
                unsigned alt_dim = tiling.getAltitudinalTiles();

                // determine tile range in azimuthal direction (modulated)
                unsigned azi_from = tiling.discreteAngle(
                        angleUnsignedNormal<Radians>(azimuth - hd_pers) ) % azi_dim;
                unsigned azi_to = (1 + tiling.discreteAngle(
                        angleUnsignedNormal<Radians>(azimuth + hd_pers) )) % azi_dim;

                // determine tile range in altitudinal direction (clamped)
                unsigned alt_from = tiling.discreteAngle(
                        max(-Radians::half_pi(),min(Radians::half_pi(), 
                        altitude - hd_pers)) + Radians::half_pi() );
                unsigned alt_to = tiling.discreteAngle(
                        max(-Radians::half_pi(),min(Radians::half_pi(), 
                        altitude + hd_pers)) + Radians::half_pi() );

                // iterate the grid...
                unsigned n_batches = 0u;

fprintf(stderr, "Stars.cpp: grid dimensions: %d x %d\n", azi_dim, alt_dim);
fprintf(stderr, "Stars.cpp: grid range: [%d;%d) [%d;%d]\n", azi_from, azi_to, alt_from, alt_to);

                GLint* offs = ptr_batch_offs, * count = ptr_batch_count;
                for (unsigned alt = alt_from; alt <= alt_to; ++alt)
                {
                    for (unsigned azi = azi_from; 
                            azi != azi_to; azi = (azi + 1) % azi_dim)
                    {
                        unsigned tile_index = azi + alt * azi_dim;
                        Tile* t = ptr_tiles + tile_index;

                        // LOD brightness changed? 
                        if (lod != val_lod)
                        {
                            // determine bounds for binary search
                            //
                            // a growing number of stars needs to be rendereed
                            // at decreasing minimum brightness - perform a
                            // binary search in the so found partition for the
                            // new vertex count in this tile

                            GpuVertex const* start = ptr_data + t[0].offset;
                            GpuVertex const* end = ptr_data + t[1].offset;

                            if (start == end)
                                continue;

                            if (lod < val_lod)
                                start += (t->count > 0 ? t->count - 1 : 0);
                            else
                                end = start + t->count;

                            end = std::upper_bound(
                                    start, end, lod, GreaterBrightness());

                            t->count = end - ptr_data + t[0].offset;
                        }

                        if (! t->count)
                            continue;

fprintf(stderr, "Stars.cpp: tile %d selected (%d vertices at offset %d)\n", tile_index, t->count, t->offset);

                        *offs++ = t->offset;
                        *count++ = t->count;

                        ++n_batches;
                    }
                }

fprintf(stderr, "Stars.cpp: rendering %d-multibatch\n", n_batches);

                // cancel translation
                local_space[3][0] = 0.0f;
                local_space[3][1] = 0.0f;
                local_space[3][2] = 0.0f;

                // and setup modelview matrix
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadMatrixf(glm::value_ptr(
                        fov.setOrientation(local_space).getWorldViewerXform()));

                // render
                glBindVertexArray(hnd_vao);
                glPointSize(1.0);
                glMultiDrawArrays(GL_POINTS,
                        ptr_batch_offs, ptr_batch_count, n_batches);

                // restore state state
                glBindVertexArray(0); 
                glPopMatrix();
            }

    };
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
        : val_tile_resolution(12), val_lod_brightness(0),
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
        float oa_fraction = min(fraction * (1.0f + oa_fraction), 1.0f);
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


