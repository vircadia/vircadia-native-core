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

#ifdef _WIN32
#include "../Config.h"
#define lrint(x) (floor(x+(x>0) ? 0.5 : -0.5)) 
inline float remainder(float x,float y) { return std::fmod(x,y); }
inline int round(float x) { return (floor(x + 0.5)); }
double log2( double n )  
{  
    // log(n)/log(2) is log2.  
    return log( n ) / log( 2 );  
}
#else
#include "../starfield/Config.h"
#endif
namespace starfield {

    class Tiling {

        unsigned    _valK;
        float       _valRcpSlice;
        unsigned    _valBits;

    public:

        Tiling(unsigned k) : 
            _valK(k),
            _valRcpSlice(k / Radians::twicePi()) {
            _valBits = ceil(log2(getTileCount()));
        }

        unsigned getAzimuthalTiles() const { return _valK; }
        unsigned getAltitudinalTiles() const { return _valK / 2 + 1; }
        unsigned getTileIndexBits() const { return _valBits; }

        unsigned getTileCount() const {
            return getAzimuthalTiles() * getAltitudinalTiles();
        }

        unsigned getTileIndex(float azimuth, float altitude) const {
            return discreteAzimuth(azimuth) +
                    _valK * discreteAltitude(altitude);
        }

        float getSliceAngle() const {
            return 1.0f / _valRcpSlice;
        }

    private:

        unsigned discreteAngle(float unsigned_angle) const {
            return unsigned(round(unsigned_angle * _valRcpSlice));
        }

        unsigned discreteAzimuth(float a) const {
            return discreteAngle(a) % _valK;
        }

        unsigned discreteAltitude(float a) const {
            return min(getAltitudinalTiles() - 1, 
                    discreteAngle(a + Radians::halfPi()) );
        }

    };

} // anonymous namespace

#endif

