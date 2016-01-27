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

Octree::Indices Octree::allocateCellPath(const CellPath& path) {
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