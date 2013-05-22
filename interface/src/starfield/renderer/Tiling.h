//
// starfield/renderer/Tiling.h 
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__renderer__Tiling__
#define __interface__starfield__renderer__Tiling__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

#include "starfield/Config.h"

namespace starfield {

    class Tiling {
    public:

        Tiling(unsigned k) : 
            _valK(k),
            _rcpSlice(k / Radians::twicePi()) {
            _nBits = ceil(log(getTileCount()) * 1.4426950408889634); // log2
        }

        unsigned getAzimuthalTiles() const { return _valK; }
        unsigned getAltitudinalTiles() const { return _valK / 2 + 1; }
        unsigned getTileIndexBits() const { return _nBits; }

        unsigned getTileCount() const {
            return getAzimuthalTiles() * getAltitudinalTiles();
        }

        unsigned getTileIndex(float azimuth, float altitude) const {
            return discreteAzimuth(azimuth) +
                    _valK * discreteAltitude(altitude);
        }

        float getSliceAngle() const {
            return 1.0f / _rcpSlice;
        }

    private:

        unsigned discreteAngle(float unsigned_angle) const {

            return unsigned(floor(unsigned_angle * _rcpSlice + 0.5f));
        }

        unsigned discreteAzimuth(float a) const {
            return discreteAngle(a) % _valK;
        }

        unsigned discreteAltitude(float a) const {
            return min(getAltitudinalTiles() - 1, 
                    discreteAngle(a + Radians::halfPi()) );
        }

        // variables

        unsigned    _valK;
        float       _rcpSlice;
        unsigned    _nBits;
    };

} // anonymous namespace

#endif

