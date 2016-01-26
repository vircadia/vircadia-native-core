//
//  Octree.h
//  render/src/render
//
//  Created by Sam Gateau on 1/25/16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Octree.h"


using namespace render;

Octree::Indices Octree::allocateCellPath(const CellPath& path) {
    

    CellPoint point{ Coord3{ 2, 4, 3 }, 3 };

    auto ppath = CellPoint::rootTo(point);

    Indices cellPath;

    Index currentIndex = 0;
    Cell* currentCell = _cells.data();
    int d = 0;
    cellPath.push_back(currentIndex);

    for (; d < path.back().depth; d++) {
        auto& cellPoint = path[d];

        auto currentIndex = currentCell->child(cellPoint.octant());
        if (currentIndex == INVALID) {
            break;
        }

        cellPath.push_back(currentIndex);
        currentCell = _cells.data() + currentIndex;
    }

    return cellPath;
}