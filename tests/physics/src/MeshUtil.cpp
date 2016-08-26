//
//  MeshUtil.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows 2016.07.14
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MeshUtil.h"

#include<unordered_map>

// returns false if any edge has only one adjacent triangle
bool MeshUtil::isClosedManifold(const uint32_t* meshIndices, uint32_t numIndices) {
    using EdgeList = std::unordered_map<MeshUtil::TriangleEdge, uint32_t>;
    EdgeList edges;

    // count the triangles for each edge
    const uint32_t TRIANGLE_STRIDE = 3;
    for (uint32_t i = 0; i < numIndices; i += TRIANGLE_STRIDE) {
        MeshUtil::TriangleEdge edge;
        // the triangles indices are stored in sequential order
        for (uint32_t j = 0; j < 3; ++j) {
            edge.setIndices(meshIndices[i + j], meshIndices[i + ((j + 1) % 3)]);

            EdgeList::iterator edgeEntry = edges.find(edge);
            if (edgeEntry == edges.end()) {
                edges.insert(std::pair<MeshUtil::TriangleEdge, uint32_t>(edge, 1));
            } else {
                edgeEntry->second += 1;
            }
        }
    }
    // scan for outside edge
    for (auto& edgeEntry : edges) {
        if (edgeEntry.second == 1) {
             return false;
        }
    }
    return true;
}

