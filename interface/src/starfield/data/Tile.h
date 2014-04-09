//
//  Tile.h
//  interface/src/starfield/data
//
//  Created by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Tile_h
#define hifi_Tile_h

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

#endif // hifi_Tile_h
