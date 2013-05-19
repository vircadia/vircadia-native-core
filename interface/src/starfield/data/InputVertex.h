//
// starfield/data/InputVertex.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__data__InputVertex__
#define __interface__starfield__data__InputVertex__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

#include "starfield/Config.h"

namespace starfield {

    class InputVertex {

        unsigned    _color;
        float       _azimuth;
        float       _altitude;
    public:

        InputVertex(float azimuth, float altitude, unsigned color) {

            _color = ((color >> 16) & 0xffu) | (color & 0xff00u) |
                    ((color << 16) & 0xff0000u) | 0xff000000u;

            azimuth = angleConvert<Degrees,Radians>(azimuth);
            altitude = angleConvert<Degrees,Radians>(altitude);

            angleHorizontalPolar<Radians>(azimuth, altitude);

            _azimuth = azimuth;
            _altitude = altitude;
        }

        float getAzimuth() const { return _azimuth; }
        float getAltitude() const { return _altitude; }
        unsigned getColor() const { return _color; } 
    }; 

    typedef std::vector<InputVertex> InputVertices;

} // anonymous namespace

#endif

