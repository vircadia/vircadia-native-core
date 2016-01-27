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


const double Octree::INV_DEPTH_DIM[] = {
    1.0,
    1.0 / 2.0,
    1.0 / 4.0,
    1.0 / 8.0,
    1.0 / 16.0,
    1.0 / 32.0,
    1.0 / 64.0,
    1.0 / 128.0,
    1.0 / 256.0,
    1.0 / 512.0,
    1.0 / 1024.0,
    1.0 / 2048.0,
    1.0 / 4096.0,
    1.0 / 8192.0,
    1.0 / 16384.0,
    1.0 / 32768.0 };


Octree::Location::vector Octree::Location::rootTo(const Location& dest) {
    Location current{ dest };
    vector path(dest.depth + 1);
    path[dest.depth] = dest;
    while (current.depth > 0) {
        current = current.parent();
        path[current.depth] = current;
    }
    return path;
}

Octree::Location Octree::Location::evalFromRange(const Coord3& minCoord, const Coord3& maxCoord, Depth rangeDepth) {
    Depth depthOffset = MAX_DEPTH - rangeDepth;
    Depth depth = depthOffset;
    Coord3 mask(depthBitmask(depth));

    while (depth < rangeDepth) {
        Coord3 nextMask = mask | depthBitmask(depth + 1);
        if ((minCoord & nextMask) != (maxCoord & nextMask)) {
            break;
        }
        mask = nextMask;
        depth++;
    }

    if (depth == 0) {
        return Location();
    } else {
        // Location depth and coordinate are found, need to bring the coordinate from sourceDepth to destinationDepth
        auto sourceCoord = (minCoord & mask);
        return Location(sourceCoord >> Coord3(rangeDepth - depth), depth);
    }
}

Octree::Indices Octree::allocateCellPath(const Locations& path) {
    Indices cellPath;

    Index currentIndex = 0;
    Cell* currentCell = _cells.data();
    cellPath.push_back(currentIndex);

    for (int d = 0; d <= path.back().depth; d++) {
        auto& location = path[d];

        auto currentIndex = currentCell->child(location.octant());
        if (currentIndex == INVALID) {
            currentIndex = _cells.size();
            currentCell->setChild(location.octant(), currentIndex);
            _cells.push_back(Cell(cellPath.back(), location));
        }
        cellPath.push_back(currentIndex);
        currentCell = _cells.data() + currentIndex;
    }

    return cellPath;
}


void ItemSpatialTree::insert(const ItemBounds& items) {
    for (auto& item : items) {
        if (!item.bound.isNull()) {
            editCell(evalLocation(item.bound));
        }
    }
}

void ItemSpatialTree::erase(const ItemBounds& items) {

}