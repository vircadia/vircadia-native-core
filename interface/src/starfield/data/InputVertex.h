//
// starfield/data/InputVertex.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Modified by Chris Barnard 10/17/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__data__InputVertex__
#define __interface__starfield__data__InputVertex__

#include "starfield/Config.h"

namespace starfield {

    class InputVertex {
    public:

        InputVertex(float azimuth, float altitude, unsigned color);

        float getAzimuth() const { return _azimuth; }
        float getAltitude() const { return _altitude; }
        unsigned getColor() const { return _color; } 

    private:
        unsigned _color;
        float _azimuth;
        float _altitude;
    }; 

    typedef std::vector<InputVertex> InputVertices;

}

#endif

