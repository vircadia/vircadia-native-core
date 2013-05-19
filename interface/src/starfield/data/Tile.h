//
// starfield/data/Tile.h
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__data__Tile__
#define __interface__starfield__data__Tile__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

#include "starfield/Config.h"
#include "starfield/data/BrightnessLevel.h"

namespace starfield {

    struct Tile {

        nuint           offset;
        nuint           count; 
        BrightnessLevel lod;
        nuint           flags;

        // flags
        static uint16_t const checked = 1;
        static uint16_t const visited = 2;
        static uint16_t const render = 4;
    };


} // anonymous namespace

#endif

