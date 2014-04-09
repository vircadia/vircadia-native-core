//
//  VertexOrder.h
//  interface/src/starfield/renderer
//
//  Created by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __interface__starfield__renderer__VertexOrder__
#define __interface__starfield__renderer__VertexOrder__

#include "starfield/Config.h"
#include "starfield/data/InputVertex.h"
#include "starfield/renderer/Tiling.h"

namespace starfield {

    // Defines the vertex order for the renderer as a bit extractor for
    //binary in-place Radix Sort.
    
    class VertexOrder : public Radix2IntegerScanner<unsigned>
    {
    public:
        explicit VertexOrder(Tiling const& tiling) :

        base(tiling.getTileIndexBits()), _tiling(tiling) { }

        bool bit(InputVertex const& vertex, state_type const& state) const;

    private:
        Tiling _tiling;

        typedef Radix2IntegerScanner<unsigned> base;
    };

} // anonymous namespace

#endif

