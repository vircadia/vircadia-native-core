//
//  VertexOrder.cpp
//  interface/src/starfield/renderer
//
//  Created by Chris Barnard on 10/17/13.
//  Based on code by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "starfield/renderer/VertexOrder.h"

using namespace starfield;

bool VertexOrder::bit(InputVertex const& vertex, state_type const& state) const {
    unsigned key = _tiling.getTileIndex(vertex.getAzimuth(), vertex.getAltitude());
    return base::bit(key, state);
}
