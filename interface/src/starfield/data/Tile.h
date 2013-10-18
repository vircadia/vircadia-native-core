//
// starfield/data/Tile.h
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__data__Tile__
#define __interface__starfield__data__Tile__

#include "starfield/Config.h"

namespace starfield {

    struct Tile {
        nuint offset;
        nuint count;
        nuint flags;

        // flags
        static uint16_t const checked = 1;
        static uint16_t const visited = 2;
        static uint16_t const render = 4;
    };
    
}

#endif

