//
//  GpuVertex.h
//  interface/src/starfield/data
//
//  Created by Tobias Schwinger on 3/29/13.
//  Modified by Chris Barnard on 10/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GpuVertex_h
#define hifi_GpuVertex_h

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

#endif // hifi_GpuVertex_h
