//
// starfield/renderer/VertexOrder.cpp
// interface
//
// Created by Chris Barnard on 10/17/13.
// Based on code by Tobias Schwinger on 3/22/13.
//
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "starfield/renderer/VertexOrder.h"

using namespace starfield;

bool VertexOrder::bit(InputVertex const& vertex, state_type const& state) const {
    unsigned key = _tiling.getTileIndex(vertex.getAzimuth(), vertex.getAltitude());
    return base::bit(key, state);
}