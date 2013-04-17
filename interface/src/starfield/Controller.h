//
// starfield/Controller.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__Controller__
#define __interface__starfield__Confroller__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

//
// Data pipeline
// =============
//
// ->> readInput -(load)--+---- (get brightness & sort) ---> brightness LUT
//                        |              |        
// ->> setResolution --+  |             >extractBrightnessLevels<
//                     V  |
//            (sort by (tile,brightness))
//                        |         |
// ->> setLOD  ---+       |    >retile<   ->> setLOD --> (just parameterize
//                V       V                          when enough data on-GPU)
//          (filter by max-LOD brightness,         
//            build tile info for rendering)
//                        |             |
//                        V         >recreateRenderer<
//                 (set new renderer)/
//
//
// (process), ->> entry point, ---> data flow, >internal routine<
//
// (member functions are ordered by data flow)

//
// Still open 
// ==========
//
// o atomics/mutexes need to be added as annotated in the source to allow
//   concurrent threads to pull the strings to e.g. have a low priority
//   thread run the data pipeline for update -- rendering is wait-free
//

#include "starfield/data/InputVertex.h"
#include "starfield/data/BrightnessLevel.h"
#include "starfield/Loader.h"

#include "starfield/renderer/Renderer.h"
#include "starfield/renderer/VertexOrder.h"

namespace starfield {

    class Controller {

        InputVertices           _seqInput;
#if STARFIELD_MULTITHREADING
        mutex                   _mtxInput;
        atomic<unsigned>        _valTileResolution;

        mutex                   _mtxLodState;
#else
        unsigned                _valTileResolution;
#endif
        double                  _valLodFraction;
        double                  _valLodLowWaterMark;
        double                  _valLodHighWaterMark;
        double                  _valLodOveralloc;
        size_t                  _valLodNalloc;
        size_t                  _valLodNrender;
        BrightnessLevels        _seqLodBrightness;

#if STARFIELD_MULTITHREADING
        atomic<BrightnessLevel> _valLodBrightness;
        BrightnessLevel         _valLodAllocBrightness;

        atomic<Renderer*>       _ptrRenderer;

        typedef lock_guard<mutex> lock;
#else
        BrightnessLevel         _valLodBrightness;
        BrightnessLevel         _valLodAllocBrightness;

        Renderer*               _ptrRenderer;

        #define lock
        #define _(x)
#endif

        static inline size_t toBufSize(double f) {
            return size_t(floor(f + 0.5f));
        }

    public:

        Controller() :
            _valTileResolution(20), 
            _valLodFraction(1.0),
            _valLodLowWaterMark(0.8),
            _valLodHighWaterMark(1.0),
            _valLodOveralloc(1.2),
            _valLodNalloc(0),
            _valLodNrender(0),
            _valLodBrightness(0),
            _valLodAllocBrightness(0),
            _ptrRenderer(0l) {
        }

        bool readInput(const char* url, unsigned limit)
        {
            InputVertices vertices;

            if (! Loader().loadVertices(vertices, url, limit))
                return false;

            BrightnessLevels brightness;
            extractBrightnessLevels(brightness, vertices);

            // input is read, now run the entire data pipeline on the new input

            {   lock _(_mtxInput);

                _seqInput.swap(vertices);
#if STARFIELD_MULTITHREADING
                unsigned k = _valTileResolution.load(memory_order_relaxed);
#else
                unsigned k = _valTileResolution;
#endif
                size_t n, nRender;
                BrightnessLevel bMin, b;
                double rcpChange;

                // we'll have to build a new LOD state for a new total N,
                // ideally keeping allocation size and number of vertices

                {   lock _(_mtxLodState); 

                    size_t newLast = _seqInput.size() - 1;

                    // reciprocal change N_old/N_new tells us how to scale
                    // the fractions
                    rcpChange = min(1.0, double(vertices.size()) / _seqInput.size());

                    // initialization? use defaults / previously set values
                    if (rcpChange == 0.0) {

                        rcpChange = 1.0;

                        nRender = toBufSize(_valLodFraction * newLast);
                        n = min(newLast, toBufSize(_valLodOveralloc * nRender));

                    } else {

                        // cannot allocate or render more than we have
                        n = min(newLast, _valLodNalloc);
                        nRender = min(newLast, _valLodNrender);
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
                    vertices.swap(_seqInput);
                    throw;
                }

                // finally publish the new LOD state

                {   lock _(_mtxLodState);

                    _seqLodBrightness.swap(brightness);
                    _valLodFraction *= rcpChange;
                    _valLodLowWaterMark *= rcpChange;
                    _valLodHighWaterMark *= rcpChange;
                    _valLodOveralloc *= rcpChange;
                    _valLodNalloc = n;
                    _valLodNrender = nRender;
                    _valLodAllocBrightness = bMin;
#if STARFIELD_MULTITHREADING
                    _valLodBrightness.store(b, memory_order_relaxed);
#else
                    _valLodBrightness = b;
#endif
                }
            }

            return true;
        }

        bool setResolution(unsigned k) {

            if (k <= 3) {
                return false;
            }

// printLog("Stars.cpp: setResolution(%d)\n", k);

#if STARFIELD_MULTITHREADING
            if (k != _valTileResolution.load(memory_order_relaxed)) 
#else
            if (k != _valTileResolution) 
#endif
            {   lock _(_mtxInput);

                unsigned n;
                BrightnessLevel b, bMin;

                {   lock _(_mtxLodState);

                    n = _valLodNalloc;
#if STARFIELD_MULTITHREADING
                    b = _valLodBrightness.load(memory_order_relaxed);
#else
                    b = _valLodBrightness;
#endif
                    bMin = _valLodAllocBrightness;
                }

                this->retile(n, k, b, bMin);

                return true;
            } else {
                return false;
            }
        } 

    private:

        void retile(size_t n, unsigned k, 
                    BrightnessLevel b, BrightnessLevel bMin) {

            Tiling tiling(k);
            VertexOrder scanner(tiling);
            radix2InplaceSort(_seqInput.begin(), _seqInput.end(), scanner);

// printLog(
//        "Stars.cpp: recreateRenderer(%d, %d, %d, %d)\n", n, k, b, bMin);
     
            recreateRenderer(n, k, b, bMin);

            _valTileResolution = k;
        }

    public:

        double changeLOD(double factor, double overalloc, double realloc) {

            assert(overalloc >= realloc && realloc >= 0.0);
            assert(overalloc <= 1.0 && realloc <= 1.0);

// printLog(
//       "Stars.cpp: changeLOD(%lf, %lf, %lf)\n", factor, overalloc, realloc);

            size_t n, nRender;
            BrightnessLevel bMin, b;
            double fraction, lwm, hwm;

            {   lock _(_mtxLodState);
        
                // acuire a consistent copy of the current LOD state
                fraction = _valLodFraction;
                lwm = _valLodLowWaterMark;
                hwm = _valLodHighWaterMark;
                size_t last = _seqLodBrightness.size() - 1;

                // apply factor
                fraction = max(0.0, min(1.0, fraction * factor));

                // calculate allocation size and corresponding brightness
                // threshold
                double oaFract = std::min(fraction * (1.0 + overalloc), 1.0);
                n = toBufSize(oaFract * last);
                bMin = _seqLodBrightness[n];
                n = std::upper_bound(
                        _seqLodBrightness.begin() + n - 1,
                        _seqLodBrightness.end(), 
                        bMin, GreaterBrightness() ) - _seqLodBrightness.begin();

                // also determine number of vertices to render and brightness
                nRender = toBufSize(fraction * last);
                // Note: nRender does not have to be accurate
                b = _seqLodBrightness[nRender];
                // this setting controls the renderer, also keep b as the 
                // brightness becomes volatile as soon as the mutex is
                // released, so keep b
#if STARFIELD_MULTITHREADING
                _valLodBrightness.store(b, memory_order_relaxed);
#else
                _valLodBrightness = b;
#endif

// printLog("Stars.cpp: "
//        "fraction = %lf, oaFract = %lf, n = %d, n' = %d, bMin = %d, b = %d\n", 
//        fraction, oaFract, toBufSize(oaFract * last)), n, bMin, b);

                // will not have to reallocate? set new fraction right away
                // (it is consistent with the rest of the state in this case)
                if (fraction >= _valLodLowWaterMark 
                        && fraction <= _valLodHighWaterMark) {

                    _valLodFraction = fraction;
                    return fraction;
                }
            }

            // reallocate

            {   lock _(_mtxInput);

                recreateRenderer(n, _valTileResolution, b, bMin); 

// printLog("Stars.cpp: LOD reallocation\n"); 
     
                // publish new lod state

                {   lock _(_mtxLodState);

                    _valLodNalloc = n;
                    _valLodNrender = nRender;

                    _valLodFraction = fraction;
                    _valLodLowWaterMark = fraction * (1.0 - realloc);
                    _valLodHighWaterMark = fraction * (1.0 + realloc);
                    _valLodOveralloc = fraction * (1.0 + overalloc);
                    _valLodAllocBrightness = bMin;
                }
            }
            return fraction;
        }

    private:

        void recreateRenderer(size_t n, unsigned k, 
                              BrightnessLevel b, BrightnessLevel bMin) {

#if STARFIELD_MULTITHREADING
            delete _ptrRenderer.exchange(new Renderer(_seqInput, n, k, b, bMin) ); 
#else
            delete _ptrRenderer;
            _ptrRenderer = new Renderer(_seqInput, n, k, b, bMin);
#endif
        }

    public:

        void render(float perspective, float angle, mat4 const& orientation) {

#if STARFIELD_MULTITHREADING
            // check out renderer
            Renderer* renderer = _ptrRenderer.exchange(0l);
#else
            Renderer* renderer = _ptrRenderer;
#endif

            // have it render
            if (renderer != 0l) {
#if STARFIELD_MULTITHREADING
                BrightnessLevel b = _valLodBrightness.load(memory_order_relaxed); 
#else
                BrightnessLevel b = _valLodBrightness; 
#endif
                renderer->render(perspective, angle, orientation, b);
            }

#if STARFIELD_MULTITHREADING
            // check in - or dispose if there is a new one
            Renderer* newOne = 0l;
            if (! _ptrRenderer.compare_exchange_strong(newOne, renderer)) {

                assert(!! newOne);
                delete renderer;
            }
#else
#   undef lock
#   undef _
#endif
        }

    private:

        struct BrightnessSortScanner : Radix2IntegerScanner<BrightnessLevel> {

            typedef Radix2IntegerScanner<BrightnessLevel> base;

            BrightnessSortScanner() : base(BrightnessBits) { }

            bool bit(BrightnessLevel const& k, state_type& s) { 

                // bit is inverted to achieve descending order
                return ! base::bit(k,s);
            }
        };

        static void extractBrightnessLevels(BrightnessLevels& dst, 
                                     InputVertices const& src) {
            dst.clear();
            dst.reserve(src.size());
            for (InputVertices::const_iterator i = 
                    src.begin(), e = src.end(); i != e; ++i)
                dst.push_back( getBrightness(i->getColor()) );

            radix2InplaceSort(dst.begin(), dst.end(), BrightnessSortScanner());
        }

    };
}


#endif
