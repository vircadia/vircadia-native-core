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

#include <ViewFrustum.h>


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
    Index currentIndex = ROOT_CELL;
    Indices cellPath(1, currentIndex);

    for (int l = 1; l < path.size(); l++) {
        auto& location = path[l];
        auto nextIndex = getConcreteCell(currentIndex).child(location.octant());
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
    auto& cell = editCell(cellID);
    if (!cell.hasBrick()) {
        if (!createBrick) {
            return INVALID;
        }
        cell.setBrick(allocateBrick());
    }

    // access the brick
    auto brickID = cell.brick();
    auto& brick = _bricks[brickID];

    // execute the accessor
    accessor(cell, brick, brickID);

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

ItemSpatialTree::Index ItemSpatialTree::insertItem(Index cellIdx, const ItemID& item) {
    // Add the item to the brick (and a brick if needed)
    auto brickID = accessCellBrick(cellIdx, [&](Cell& cell, Brick& brick, Octree::Index cellID) {
        brick.items.push_back(item);
        cell.signalBrickFilled();
    }, true);

    return cellIdx;
}

bool ItemSpatialTree::removeItem(Index cellIdx, const ItemID& item) {
    auto success = false;

    // Access the brick at the cell (without createing new ones)
    accessCellBrick(cellIdx, [&](Cell& cell, Brick& brick, Octree::Index brickID) {
        // remove the item from the list
        brick.items.erase(std::find(brick.items.begin(), brick.items.end(), item));
        if (brick.items.empty()) {
            cell.signalBrickEmpty();
        }
        success = true;
    }, false); // do not create brick!

    return success;
}

ItemSpatialTree::Index ItemSpatialTree::resetItem(Index oldCell, const Location& location, const ItemID& item) {
    auto newCell = indexCell(location);

    // Early exit if nothing changed
    if (newCell == oldCell) {
        return newCell;
    }
    // do we know about this item ?
    else if (oldCell == Item::INVALID_CELL) {
        insertItem(newCell, item);
        return newCell;
    }
    // A true update of cell is required
    else {
        // Add the item to the brick (and a brick if needed)
        accessCellBrick(newCell, [&](Cell& cell, Brick& brick, Octree::Index cellID) {
            brick.items.push_back(item);
            cell.signalBrickFilled();
        }, true);

        // And remove it from the previous one
        removeItem(oldCell, item);

        return newCell;
    }
}

int ItemSpatialTree::select(ItemIDs& selection, const ViewFrustum& frustum) const {
    auto worldPlanes = frustum.getPlanes();
    Coord4f planes[6];
    for (int i = 0; i < ViewFrustum::NUM_PLANES; i++) {
        ::Plane octPlane;
        octPlane.setNormalAndPoint(worldPlanes[i].getNormal(), evalCoordf(worldPlanes[i].getPoint(), ROOT_DEPTH));
        planes[i] = Coord4f(octPlane.getNormal(), octPlane.getDCoefficient());
    }
    return Octree::select(selection, planes);
}

int Octree::select(ItemIDs& selection, const glm::vec4 frustum[6]) const {

    Index cellID = ROOT_CELL;
    selectTraverse(cellID, selection, frustum);
    return selection.size();
}

int Octree::selectTraverse(Index cellID, ItemIDs& selection, const Coord4f frustum[6]) const {
    int numSelectedsIn = selection.size();
    auto cell = getConcreteCell(cellID);

    auto intersection = Octree::Location::intersectCell(cell.getlocation(), frustum);
    switch (intersection) {
        case Octree::Location::Outside:
            // cell is outside, stop traversing this branch
            return 0;
            break;
        case Octree::Location::Inside: {
            // traverse all the Cell Branch and collect items in the selection
            selectBranch(cellID, selection, frustum);
            break;
        }
        case Octree::Location::Intersect:
        default: {
            // Cell is partially in
            selection.push_back(cellID);

            // Collect the items of this cell

            // then traverse further
            for (int i = 0; i < NUM_OCTANTS; i++) {
                Index subCellID = cell.child((Link)i);
                if (subCellID != INVALID) {
                    selectTraverse(subCellID, selection, frustum);
                }
            }
        }
    }

    return selection.size() - numSelectedsIn;
}


int  Octree::selectBranch(Index cellID, ItemIDs& selection, const Coord4f frustum[6]) const {
    int numSelectedsIn = selection.size();
    auto cell = getConcreteCell(cellID);

    // Collect the items of this cell
    selection.push_back(cellID);

    // then traverse further
    for (int i = 0; i < NUM_OCTANTS; i++) {
        Index subCellID = cell.child((Link)i);
        if (subCellID != INVALID) {
            selectBranch(subCellID, selection, frustum);
        }
    }

    return selection.size() - numSelectedsIn;
}

Octree::Location::Intersection Octree::Location::intersectCell(const Location& cell, const Coord4f frustum[6]) {
    const Coord3f CornerOffsets[8] = {
        { 0.0, 0.0, 0.0 },
        { 1.0, 0.0, 0.0 },
        { 0.0, 1.0, 0.0 },
        { 1.0, 1.0, 0.0 },
        { 0.0, 0.0, 1.0 },
        { 1.0, 0.0, 1.0 },
        { 0.0, 1.0, 1.0 },
        { 1.0, 1.0, 1.0 },
    };

    struct Tool {
        static int normalToIndex(const Coord3f& n) {
            int index = 0;
            if (n.x >= 0.0) index |= 1;
            if (n.y >= 0.0) index |= 2;
            if (n.z >= 0.0) index |= 4;
            return index;
        }

        static bool halfPlaneTest(const Coord4f& plane, const Coord3f& pos) {
            return (glm::dot(plane, Coord4f(pos, 1.0f)) >= 0.0f);
        }
    };

    Coord3f cellSize = Coord3f(Octree::getInvDepthDimension(cell.depth));
    Coord3f cellPos = Coord3f(cell.pos) * cellSize;

    bool partialFlag = false;
    for (int p = 0; p < ViewFrustum::NUM_PLANES; p++) {
        Coord4f plane = frustum[p];
        Coord3f planeNormal(plane);

        int index = Tool::normalToIndex(planeNormal);

        auto negTestPoint = cellPos + cellSize * CornerOffsets[index];

        if (!Tool::halfPlaneTest(plane, negTestPoint)) {
            return Outside;
        }

        index = Tool::normalToIndex(-planeNormal);

        auto posTestPoint = cellPos + cellSize * CornerOffsets[index];

        if (!Tool::halfPlaneTest(plane, posTestPoint)) {
            partialFlag = true;
        }
    }

    if (partialFlag) {
        return Intersect;
    }

    return Inside;
}