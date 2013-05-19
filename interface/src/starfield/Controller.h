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

        InputVertices           _inputSequence;
#if STARFIELD_MULTITHREADING
        mutex                   _inputMutex;
        atomic<unsigned>        _tileResolution;

        mutex                   _lodStateMutex;
#else
        unsigned                _tileResolution;
#endif
        double                  _lodFraction;
        double                  _lodLowWaterMark;
        double                  _lodHighWaterMark;
        double                  _lodOveralloc;
        size_t                  _lodNalloc;
        size_t                  _lodNRender;
        BrightnessLevels        _lodBrightnessSequence;

#if STARFIELD_MULTITHREADING
        atomic<BrightnessLevel> _lodBrightness;
        BrightnessLevel         _lodAllocBrightness;

        atomic<Renderer*>       _renderer;

        typedef lock_guard<mutex> lock;
#else
        BrightnessLevel         _lodBrightness;
        BrightnessLevel         _lodAllocBrightness;

        Renderer*               _renderer;

        #define lock
        #define _(x)
#endif

        static inline size_t toBufSize(double f) {
            return size_t(floor(f + 0.5f));
        }

    public:

        Controller() :
            _tileResolution(20), 
            _lodFraction(1.0),
            _lodLowWaterMark(0.8),
            _lodHighWaterMark(1.0),
            _lodOveralloc(1.2),
            _lodNalloc(0),
            _lodNRender(0),
            _lodBrightness(0),
            _lodAllocBrightness(0),
            _renderer(0l) {
        }

        bool readInput(const char* url, const char* cacheFile, unsigned limit)
        {
            InputVertices vertices;

            if (! Loader().loadVertices(vertices, url, cacheFile, limit))
                return false;

            BrightnessLevels brightness;
            extractBrightnessLevels(brightness, vertices);

            // input is read, now run the entire data pipeline on the new input

            {   lock _(_inputMutex);

                _inputSequence.swap(vertices);
#if STARFIELD_MULTITHREADING
                unsigned k = _tileResolution.load(memory_order_relaxed);
#else
                unsigned k = _tileResolution;
#endif
                size_t n, nRender;
                BrightnessLevel bMin, b;
                double rcpChange;

                // we'll have to build a new LOD state for a new total N,
                // ideally keeping allocation size and number of vertices

                {   lock _(_lodStateMutex); 

                    size_t newLast = _inputSequence.size() - 1;

                    // reciprocal change N_old/N_new tells us how to scale
                    // the fractions
                    rcpChange = min(1.0, double(vertices.size()) / _inputSequence.size());

                    // initialization? use defaults / previously set values
                    if (rcpChange == 0.0) {

                        rcpChange = 1.0;

                        nRender = toBufSize(_lodFraction * newLast);
                        n = min(newLast, toBufSize(_lodOveralloc * nRender));

                    } else {

                        // cannot allocate or render more than we have
                        n = min(newLast, _lodNalloc);
                        nRender = min(newLast, _lodNRender);
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
                    vertices.swap(_inputSequence);
                    throw;
                }

                // finally publish the new LOD state

                {   lock _(_lodStateMutex);

                    _lodBrightnessSequence.swap(brightness);
                    _lodFraction *= rcpChange;
                    _lodLowWaterMark *= rcpChange;
                    _lodHighWaterMark *= rcpChange;
                    _lodOveralloc *= rcpChange;
                    _lodNalloc = n;
                    _lodNRender = nRender;
                    _lodAllocBrightness = bMin;
#if STARFIELD_MULTITHREADING
                    _lodBrightness.store(b, memory_order_relaxed);
#else
                    _lodBrightness = b;
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
            if (k != _tileResolution.load(memory_order_relaxed)) 
#else
            if (k != _tileResolution) 
#endif
            {   lock _(_inputMutex);

                unsigned n;
                BrightnessLevel b, bMin;

                {   lock _(_lodStateMutex);

                    n = _lodNalloc;
#if STARFIELD_MULTITHREADING
                    b = _lodBrightness.load(memory_order_relaxed);
#else
                    b = _lodBrightness;
#endif
                    bMin = _lodAllocBrightness;
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
            radix2InplaceSort(_inputSequence.begin(), _inputSequence.end(), scanner);

// printLog(
//        "Stars.cpp: recreateRenderer(%d, %d, %d, %d)\n", n, k, b, bMin);
     
            recreateRenderer(n, k, b, bMin);

            _tileResolution = k;
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

            {   lock _(_lodStateMutex);
        
                // acuire a consistent copy of the current LOD state
                fraction = _lodFraction;
                lwm = _lodLowWaterMark;
                hwm = _lodHighWaterMark;
                size_t last = _lodBrightnessSequence.size() - 1;

                // apply factor
                fraction = max(0.0, min(1.0, fraction * factor));

                // calculate allocation size and corresponding brightness
                // threshold
                double oaFract = std::min(fraction * (1.0 + overalloc), 1.0);
                n = toBufSize(oaFract * last);
                bMin = _lodBrightnessSequence[n];
                n = std::upper_bound(
                        _lodBrightnessSequence.begin() + n - 1,
                        _lodBrightnessSequence.end(), 
                        bMin, GreaterBrightness() ) - _lodBrightnessSequence.begin();

                // also determine number of vertices to render and brightness
                nRender = toBufSize(fraction * last);
                // Note: nRender does not have to be accurate
                b = _lodBrightnessSequence[nRender];
                // this setting controls the renderer, also keep b as the 
                // brightness becomes volatile as soon as the mutex is
                // released, so keep b
#if STARFIELD_MULTITHREADING
                _lodBrightness.store(b, memory_order_relaxed);
#else
                _lodBrightness = b;
#endif

// printLog("Stars.cpp: "
//        "fraction = %lf, oaFract = %lf, n = %d, n' = %d, bMin = %d, b = %d\n", 
//        fraction, oaFract, toBufSize(oaFract * last)), n, bMin, b);

                // will not have to reallocate? set new fraction right away
                // (it is consistent with the rest of the state in this case)
                if (fraction >= _lodLowWaterMark 
                        && fraction <= _lodHighWaterMark) {

                    _lodFraction = fraction;
                    return fraction;
                }
            }

            // reallocate

            {   lock _(_inputMutex);

                recreateRenderer(n, _tileResolution, b, bMin); 

// printLog("Stars.cpp: LOD reallocation\n"); 
     
                // publish new lod state

                {   lock _(_lodStateMutex);

                    _lodNalloc = n;
                    _lodNRender = nRender;

                    _lodFraction = fraction;
                    _lodLowWaterMark = fraction * (1.0 - realloc);
                    _lodHighWaterMark = fraction * (1.0 + realloc);
                    _lodOveralloc = fraction * (1.0 + overalloc);
                    _lodAllocBrightness = bMin;
                }
            }
            return fraction;
        }

    private:

        void recreateRenderer(size_t n, unsigned k, 
                              BrightnessLevel b, BrightnessLevel bMin) {

#if STARFIELD_MULTITHREADING
            delete _renderer.exchange(new Renderer(_inputSequence, n, k, b, bMin) ); 
#else
            delete _renderer;
            _renderer = new Renderer(_inputSequence, n, k, b, bMin);
#endif
        }

    public:

        void render(float perspective, float angle, mat4 const& orientation, float alpha) {

#if STARFIELD_MULTITHREADING
            // check out renderer
            Renderer* renderer = _renderer.exchange(0l);
#else
            Renderer* renderer = _renderer;
#endif

            // have it render
            if (renderer != 0l) {
#if STARFIELD_MULTITHREADING
                BrightnessLevel b = _lodBrightness.load(memory_order_relaxed); 
#else
                BrightnessLevel b = _lodBrightness; 
#endif
                renderer->render(perspective, angle, orientation, b, alpha);
            }

#if STARFIELD_MULTITHREADING
            // check in - or dispose if there is a new one
            Renderer* newOne = 0l;
            if (! _renderer.compare_exchange_strong(newOne, renderer)) {

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
