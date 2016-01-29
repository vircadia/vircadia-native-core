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


Octree::Location::vector Octree::Location::pathTo(const Location& dest) {
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

Octree::Indices Octree::indexConcreteCellPath(const Locations& path) const {
    Index currentIndex = ROOT;
    Indices cellPath(1, currentIndex);

    for (int l = 1; l < path.size(); l++) {
        auto& location = path[l];
        auto nextIndex = getCell(currentIndex).child(location.octant());
        if (nextIndex == INVALID) {
            break;
        }

        // One more cell index on the path, moving on
        currentIndex = nextIndex;
        cellPath.push_back(currentIndex);
    }

    return cellPath;
}

Octree::Index Octree::allocateCell(Index parent, const Location& location) {

    if (_cells[parent].asChild(location.octant())) {
        assert(_cells[parent].child(location.octant()) == INVALID);
        return _cells[parent].child(location.octant());
    }

    assert(_cells[parent].getlocation().child(location.octant()) == location);

    auto newIndex = _cells.size();
    _cells.push_back(Cell(parent, location));
    _cells[parent].setChild(location.octant(), newIndex);

    return newIndex;
}


Octree::Indices Octree::indexCellPath(const Locations& path) {
    // First through the aallocated cells
    Indices cellPath = indexConcreteCellPath(path);

    // Catch up from the last allocated cell on the path
    auto currentIndex = cellPath.back();

    for (int l = cellPath.size(); l < path.size(); l++) {
        auto& location = path[l];

        // Allocate the new index & connect it to the parent
        auto newIndex = allocateCell(currentIndex, location);

        // One more cell index on the path, moving on
        currentIndex = newIndex;
        cellPath.push_back(currentIndex);
    }

    return cellPath;
}


Octree::Index Octree::allocateBrick() {
    Index brickIdx = _bricks.size();
    _bricks.push_back(Brick());
    return brickIdx;
}

Octree::Index Octree::accessCellBrick(Index cellID, const CellBrickAccessor& accessor, bool createBrick) {
    assert(cellID != INVALID);
    auto cell = editCell(cellID);
    if (!cell.asBrick()) {
        if (!createBrick) {
            return INVALID;
        }
        cell.setBrick(allocateBrick());
    }

    // access the brick
    auto brickID = cell.brick();
    auto& brick = _bricks[brickID];

    // execute the accessor
    accessor(brick, brickID);

    return brickID;
}

Octree::Locations ItemSpatialTree::evalLocations(const ItemBounds& bounds) const {
    Locations locations;
    locations.reserve(bounds.size());
    for (auto& bound : bounds) {
        if (!bound.bound.isNull()) {
            locations.emplace_back(evalLocation(bound.bound));
        } else {
            locations.emplace_back(Location());
        }
    }
    return locations;
}

ItemSpatialTree::Index ItemSpatialTree::insertItem(const Location& location, const ItemID& item) {
    // Go to the cell
    auto cellIdx = indexCell(location);

    // Add the item to the brick (and a brick if needed)
    accessCellBrick(cellIdx, [&](Brick& brick, Octree::Index cellID) {
        brick.items.push_back(item);
    }, true);

    return cellIdx;
}

bool ItemSpatialTree::removeItem(Index cellIdx, const ItemID& item) {
    auto success = false;

    // Access the brick at the cell (without createing new ones)
    auto brickIdx = accessCellBrick(cellIdx, [&](Brick& brick, Octree::Index brickID) {
        // remove the item from the list
        brick.items.erase(std::find(brick.items.begin(), brick.items.end(), item));
        success = true;
    }, false); // do not create brick!

    return success;
}

ItemSpatialTree::Index ItemSpatialTree::resetItem(Index oldCell, const Location& location, const ItemID& item) {
    // do we know about this item ?
    if (oldCell == Item::INVALID_CELL) {
        auto newCell = insertItem(location, item);
        return newCell;
    } else {
        auto newCell = indexCell(location);

        accessCellBrick(newCell, [&](Brick& brick, Octree::Index brickID) {
            // insert the item only if the new cell is different from the previous one
            if (newCell != oldCell) {
                brick.items.push_back(item);
            }
        }, true);

        // now we know where the cell has been added and where it was,
        // if different then go clean the previous cell
        if (newCell != oldCell) {
            removeItem(oldCell, item);
        }

        return newCell;
    }
}

