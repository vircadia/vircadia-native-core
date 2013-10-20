//
// starfield/renderer/VertexOrder.h
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
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

