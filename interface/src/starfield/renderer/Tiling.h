//
//  Tiling.h
//  interface/src/starfield/renderer
//
//  Created by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Tiling_h
#define hifi_Tiling_h

#include "starfield/Config.h"

namespace starfield {
    const float LOG2 = 1.4426950408889634f;
    
    class Tiling {
    public:
        Tiling(unsigned tileResolution) : _tileResolution(tileResolution), _rcpSlice(tileResolution / Radians::twicePi()) {
                                    _nBits = ceil(log((float)getTileCount()) * LOG2); }
        
        unsigned getAzimuthalTiles() const { return _tileResolution; }
        unsigned getAltitudinalTiles() const { return _tileResolution / 2 + 1; }
        unsigned getTileIndexBits() const { return _nBits; }
        unsigned getTileCount() const { return getAzimuthalTiles() * getAltitudinalTiles(); }
        unsigned getTileIndex(float azimuth, float altitude) const { return discreteAzimuth(azimuth) +
                                                                     _tileResolution * discreteAltitude(altitude); }
        float getSliceAngle() const { return 1.0f / _rcpSlice; }

    private:

        unsigned discreteAngle(float unsigned_angle) const { return unsigned(floor(unsigned_angle * _rcpSlice + 0.5f)); }
        unsigned discreteAzimuth(float angle) const { return discreteAngle(angle) % _tileResolution; }
        unsigned discreteAltitude(float angle) const { return min( getAltitudinalTiles() - 1,
                                                       discreteAngle(angle + Radians::halfPi()) ); }

        // variables

        unsigned _tileResolution;
        float _rcpSlice;
        unsigned _nBits;
    };
}

#endif // hifi_Tiling_h
