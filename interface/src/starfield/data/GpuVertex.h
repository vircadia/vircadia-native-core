//
// starfield/data/GpuVertex.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__data__GpuVertex__
#define __interface__starfield__data__GpuVertex__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

#include "starfield/data/InputVertex.h"

namespace starfield {

    class GpuVertex {
    public:
        GpuVertex() { }

        GpuVertex(InputVertex const& in) {

            _color = in.getColor();
            float azi = in.getAzimuth();
            float alt = in.getAltitude();

            // ground vector in x/z plane...
            float gx =  sin(azi);
            float gz = -cos(azi);

            // ...elevated in y direction by altitude
            float exz = cos(alt);
            _valX = gx * exz;
            _valY = sin(alt);
            _valZ = gz * exz;
        }

        unsigned getColor() const { return _color; }
 
    private:
        unsigned    _color;
        float       _valX;
        float       _valY;
        float       _valZ;
    };

} // anonymous namespace

#endif

