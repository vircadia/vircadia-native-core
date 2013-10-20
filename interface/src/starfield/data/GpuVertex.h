//
// starfield/data/GpuVertex.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Modified 10/17/13 Chris Barnard.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__data__GpuVertex__
#define __interface__starfield__data__GpuVertex__

#include "starfield/data/InputVertex.h"

namespace starfield {

    class GpuVertex {
    public:
        GpuVertex() { }

        GpuVertex(InputVertex const& inputVertex);

        unsigned getColor() const { return _color; }
 
    private:
        unsigned _color;
        float _valX;
        float _valY;
        float _valZ;
    };

}

#endif

