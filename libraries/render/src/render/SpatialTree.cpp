//
//  SpatialTree.h
//  render/src/render
//
//  Created by Sam Gateau on 1/25/16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SpatialTree.h"

#include <ViewFrustum.h>


using namespace render;


const float Octree::INV_DEPTH_DIM[] = {
    1.0f,
    1.0f / 2.0f,
    1.0f / 4.0f,
    1.0f / 8.0f,
    1.0f / 16.0f,
    1.0f / 32.0f,
    1.0f / 64.0f,
    1.0f / 128.0f,
    1.0f / 256.0f,
    1.0f / 512.0f,
    1.0f / 1024.0f,
    1.0f / 2048.0f,
    1.0f / 4096.0f,
    1.0f / 8192.0f,
    1.0f / 16384.0f,
    1.0f / 32768.0f };

/*
const float Octree::COORD_SUBCELL_WIDTH[] = { // 2 ^ MAX_DEPTH / 2 ^ (depth + 1)
    16384.0f,
    8192.0f,
    4096.0f,
    2048.0f,
    1024.0f,
    512.0f,
    256.0f,
    128.0f,
    64.0f,
    32.0f,
    16.0f,
    8.0f,
    4.0f,
    2.0f,
    1.0f,
    0.5f };
*/

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

    // Start the path after the root cell so at #1
    for (size_t p = 1; p < path.size(); p++) {
        auto& location = path[p];
        auto nextIndex = getConcreteCell(currentIndex).child(location.octant());
        if (nextIndex == INVALID_CELL) {
            break;
        }

        // One more cell index on the path, moving on
        currentIndex = nextIndex;
        cellPath.push_back(currentIndex);
    }

    return cellPath;
}

Octree::Index Octree::allocateCell(Index parent, const Location& location) {

    if (_cells[parent].hasChild(location.octant())) {
        return _cells[parent].child(location.octant());
    }
    assert(_cells[parent].getlocation().child(location.octant()) == location);

    Index newIndex;
    if (_freeCells.empty()) {
        newIndex = (Index)_cells.size();
        if (newIndex >= MAXIMUM_INDEX) {
            // abort! we are trying to go overboard with the total number of allocated bricks
            return INVALID_CELL;
        }
        _cells.push_back(Cell(parent, location));
    } else {
        newIndex = _freeCells.back();
        _freeCells.pop_back();
        _cells[newIndex] = Cell(parent, location);
    }

    _cells[parent].setChild(location.octant(), newIndex);
    return newIndex;
}


void Octree::freeCell(Index index) {
    if (checkCellIndex(index)) {
        auto & cell = _cells[index];
        cell.free();
        _freeCells.push_back(index);
    }
}

void Octree::cleanCellBranch(Index index) {
    auto& cell = editCell(index);
    
    // Free the brick
    if (cell.isBrickEmpty()) {
        if (cell.hasBrick()) {
            freeBrick(cell.brick());
            cell.setBrick(INVALID_CELL);
        }
    } else {
        // If the brick is still filled, stop clearing
        return;
    }


    // Free the cell ?
    Index parentIdx = cell.parent();
    if (!cell.hasParent()) {
        if (index == ROOT_CELL) {
            // Stop here, this is the root cell!
            return;
        } else {
            // This is not expected
            assert(false);
            return;
        }
    }

    bool traverseParent = false;
    if (!cell.hasChildren()) {
        editCell(parentIdx).setChild(cell.getlocation().octant(), INVALID_CELL);
        freeCell(index);
        traverseParent = true;
    }

    if (traverseParent) {
        cleanCellBranch(parentIdx);
    }
}

Octree::Indices Octree::indexCellPath(const Locations& path) {
    // First through the allocated cells
    Indices cellPath = indexConcreteCellPath(path);

    // Catch up from the last allocated cell on the path
    auto currentIndex = cellPath.back();

    for (int l = (Index) cellPath.size(); l < (Index) path.size(); l++) {
        auto& location = path[l];

        // Allocate the new index & connect it to the parent
        auto newIndex = allocateCell(currentIndex, location);

        // One more cell index on the path, moving on
        currentIndex = newIndex;
        cellPath.push_back(currentIndex);

        // Except !!! if we actually couldn't allocate anymore
        if (newIndex == INVALID_CELL) {
            // no more cellID available, stop allocating
            // THe last index added is INVALID_CELL so the caller will know we failed allocating everything
            break;
        }
    }

    return cellPath;
}

Octree::Index Octree::allocateBrick() {
    if (_freeBricks.empty()) {
        Index brickIdx = (int)_bricks.size();
        if (brickIdx >= MAXIMUM_INDEX) {
            // abort! we are trying to go overboard with the total number of allocated bricks
            assert(false);
            // This should never happen because Bricks are allocated along with the cells and there
            // is already a cap on the cells allocation
            return INVALID_CELL;
        }
        _bricks.push_back(Brick());
        return brickIdx;
    } else {
        Index brickIdx = _freeBricks.back();
        _freeBricks.pop_back();
        return brickIdx;
    }
}

void Octree::freeBrick(Index index) {
    if (checkBrickIndex(index)) {
        auto & brick = _bricks[index];
        brick.free();
        _freeBricks.push_back(index);
    }
}

Octree::Index Octree::accessCellBrick(Index cellID, const CellBrickAccessor& accessor, bool createBrick) {
    assert(checkCellIndex(cellID));
    auto& cell = editCell(cellID);

    // Allocate a brick if needed
    if (!cell.hasBrick()) {
        if (!createBrick) {
            return INVALID_CELL;
        }
        auto newBrick = allocateBrick();
        if (newBrick == INVALID_CELL) {
            // This should never happen but just in case...
            return INVALID_CELL;
        }
        cell.setBrick(newBrick);
    }

    // Access the brick
    auto brickID = cell.brick();
    auto& brick = _bricks[brickID];

    // Execute the accessor
    accessor(cell, brick, brickID);

    return brickID;
}

Octree::Location ItemSpatialTree::evalLocation(const AABox& bound, Coord3f& minCoordf, Coord3f& maxCoordf) const {
    minCoordf = evalCoordf(bound.getMinimumPoint());
    maxCoordf = evalCoordf(bound.getMaximumPoint());

    // If the bound crosses any of the octree volume limit, then return root cell
    if (   (minCoordf.x < 0.0f)
        || (minCoordf.y < 0.0f)
        || (minCoordf.z < 0.0f)
        || (maxCoordf.x >= _size)
        || (maxCoordf.y >= _size)
        || (maxCoordf.z >= _size)) {
        return Location();
    }

    Coord3 minCoord(minCoordf);
    Coord3 maxCoord(maxCoordf);
    return Location::evalFromRange(minCoord, maxCoord);
}

Octree::Locations ItemSpatialTree::evalLocations(const ItemBounds& bounds) const {
    Locations locations;
    Coord3f minCoordf, maxCoordf;

    locations.reserve(bounds.size());
    for (auto& bound : bounds) {
        if (!bound.bound.isNull()) {
            locations.emplace_back(evalLocation(bound.bound, minCoordf, maxCoordf));
        } else {
            locations.emplace_back(Location());
        }
    }
    return locations;
}

ItemSpatialTree::Index ItemSpatialTree::insertItem(Index cellIdx, const ItemKey& key, const ItemID& item) {
    // Add the item to the brick (and a brick if needed)
    accessCellBrick(cellIdx, [&](Cell& cell, Brick& brick, Octree::Index cellID) {
        auto& itemIn = (key.isSmall() ? brick.subcellItems : brick.items);

        itemIn.push_back(item);

        cell.setBrickFilled();
    }, true);

    return cellIdx;
}

bool ItemSpatialTree::updateItem(Index cellIdx, const ItemKey& oldKey, const ItemKey& key, const ItemID& item) {
    // In case we missed that one, nothing to do
    if (cellIdx == INVALID_CELL) {
        return true;
    }
    auto success = false;

    // only if key changed
    assert(oldKey != key);

    // Get to the brick where the item is and update where it s stored
    accessCellBrick(cellIdx, [&](Cell& cell, Brick& brick, Octree::Index cellID) {
        auto& itemIn = (key.isSmall() ? brick.subcellItems : brick.items);
        auto& itemOut = (oldKey.isSmall() ? brick.subcellItems : brick.items);

        itemIn.push_back(item);
        itemOut.erase(std::find(itemOut.begin(), itemOut.end(), item));
    }, false); // do not create brick!

    return success;
}

bool ItemSpatialTree::removeItem(Index cellIdx, const ItemKey& key, const ItemID& item) {
    // In case we missed that one, nothing to do
    if (cellIdx == INVALID_CELL) {
        return true;
    }
    auto success = false;

    // Remove the item from the brick
    bool emptyCell = false;
    accessCellBrick(cellIdx, [&](Cell& cell, Brick& brick, Octree::Index brickID) {
        auto& itemList = (key.isSmall() ? brick.subcellItems : brick.items);

        itemList.erase(std::find(itemList.begin(), itemList.end(), item));

        if (brick.items.empty() && brick.subcellItems.empty()) {
            cell.setBrickEmpty();
            emptyCell = true;
        }
        success = true;
    }, false); // do not create brick!

    // Because we know the cell is now empty, lets try to clean the octree here
    if (emptyCell) {
        cleanCellBranch(cellIdx);
    }

    return success;
}

ItemSpatialTree::Index ItemSpatialTree::resetItem(Index oldCell, const ItemKey& oldKey, const AABox& bound, const ItemID& item, ItemKey& newKey) {
    auto newCell = INVALID_CELL;
    if (!newKey.isViewSpace()) {
        Coord3f minCoordf, maxCoordf;
        auto location = evalLocation(bound, minCoordf, maxCoordf);

        // Compare range size vs cell location size and tag itemKey accordingly
        // If Item bound fits in sub cell then tag as small
        auto rangeSizef = maxCoordf - minCoordf;
        float cellHalfSize = 0.5f * getCellWidth(location.depth);
        bool subcellItem = std::max(std::max(rangeSizef.x, rangeSizef.y), rangeSizef.z) < cellHalfSize;
        if (subcellItem) {
            newKey.setSmaller(subcellItem);
        } else {
            newKey.setSmaller(false);
        }

        newCell = indexCell(location);
    } else {
        // A very rare case, if we were adding items with boundary semantic expressed in view space
    }

    // Did we fail finding a cell for the item?
    if (newCell == INVALID_CELL) {
        // Remove the item where it was
        if (oldCell != INVALID_CELL) {
            removeItem(oldCell, oldKey, item);
        }
        return newCell;
    }
    // Staying in the same cell
    else if (newCell == oldCell) {
        // Did the key changed, if yes update
        if (newKey._flags != oldKey._flags) {
            updateItem(newCell, oldKey, newKey, item);
            return newCell;
        }
        return newCell;
    }
    // do we know about this item ?
    else if (oldCell == INVALID_CELL) {
        insertItem(newCell, newKey, item);
        return newCell;
    }
    // A true update of cell is required
    else {
        // Add the item to the brick (and a brick if needed)
        insertItem(newCell, newKey, item);

        // And remove it from the previous one
        removeItem(oldCell, oldKey, item);

        return newCell;
    }
}

int Octree::select(CellSelection& selection, const FrustumSelector& selector) const {

    Index cellID = ROOT_CELL;
    auto cell = getConcreteCell(cellID);
    int numSelectedsIn = (int)selection.size();

    // Always include the root cell partially containing potentially outer objects
    selectCellBrick(cellID, selection, false);

    // then traverse deeper
    for (int i = 0; i < NUM_OCTANTS; i++) {
        Index subCellID = cell.child((Link)i);
        if (subCellID != INVALID_CELL) {
            selectTraverse(subCellID, selection, selector);
        }
    }

    return (int)selection.size() - numSelectedsIn;
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
            if (n.x >= 0.0f) index |= 1;
            if (n.y >= 0.0f) index |= 2;
            if (n.z >= 0.0f) index |= 4;
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

int Octree::selectTraverse(Index cellID, CellSelection& selection, const FrustumSelector& selector) const {
    int numSelectedsIn = (int) selection.size();
    auto cell = getConcreteCell(cellID);

    auto cellLocation = cell.getlocation();

    auto intersection = Octree::Location::intersectCell(cellLocation, selector.frustum);
    switch (intersection) {
        case Octree::Location::Outside:
            // cell is outside, stop traversing this branch
            return 0;
            break;
        case Octree::Location::Inside: {
            // traverse all the Cell Branch and collect items in the selection
            selectBranch(cellID, selection, selector);
            break;
        }
        case Octree::Location::Intersect:
        default: {
            // Cell is partially in

            // Test for lod
            auto cellLocation = cell.getlocation();
            float lod = selector.testSolidAngle(cellLocation.getCenter(), Octree::getCoordSubcellWidth(cellLocation.depth));
            if (lod < 0.0f) {
                return 0;
                break;
            }

            // Select this cell partially in frustum
            selectCellBrick(cellID, selection, false);

            // then traverse deeper
            for (int i = 0; i < NUM_OCTANTS; i++) {
                Index subCellID = cell.child((Link)i);
                if (subCellID != INVALID_CELL) {
                    selectTraverse(subCellID, selection, selector);
                }
            }
        }
    }

    return (int) selection.size() - numSelectedsIn;
}


int  Octree::selectBranch(Index cellID, CellSelection& selection, const FrustumSelector& selector) const {
    int numSelectedsIn = (int) selection.size();
    auto cell = getConcreteCell(cellID);

    auto cellLocation = cell.getlocation();
    float lod = selector.testSolidAngle(cellLocation.getCenter(), Octree::getCoordSubcellWidth(cellLocation.depth));
    if (lod < 0.0f) {
        return 0;
    }

    // Select this cell fully inside the frustum
    selectCellBrick(cellID, selection, true);

    // then traverse deeper
    for (int i = 0; i < NUM_OCTANTS; i++) {
        Index subCellID = cell.child((Link)i);
        if (subCellID != INVALID_CELL) {
            selectBranch(subCellID, selection, selector);
        }
    }

    return (int) selection.size() - numSelectedsIn;
}

int Octree::selectCellBrick(Index cellID, CellSelection& selection, bool inside) const {
    int numSelectedsIn = (int) selection.size();
    auto cell = getConcreteCell(cellID);
    selection.cells(inside).push_back(cellID);

    if (!cell.isBrickEmpty()) {
        // Collect the items of this cell
        selection.bricks(inside).push_back(cell.brick());
    }

    return (int) selection.size() - numSelectedsIn;
}


int ItemSpatialTree::selectCells(CellSelection& selection, const ViewFrustum& frustum, float lodAngle) const {
    auto worldPlanes = frustum.getPlanes();
    FrustumSelector selector;
    for (int i = 0; i < ViewFrustum::NUM_PLANES; i++) {
        ::Plane octPlane;
        octPlane.setNormalAndPoint(worldPlanes[i].getNormal(), evalCoordf(worldPlanes[i].getPoint(), ROOT_DEPTH));
        selector.frustum[i] = Coord4f(octPlane.getNormal(), octPlane.getDCoefficient());
    }

    selector.eyePos = evalCoordf(frustum.getPosition(), ROOT_DEPTH);
    selector.setAngle(glm::radians(lodAngle));

    return Octree::select(selection, selector);
}

int ItemSpatialTree::selectCellItems(ItemSelection& selection, const ItemFilter& filter, const ViewFrustum& frustum, float lodAngle) const {
    selectCells(selection.cellSelection, frustum, lodAngle);

    // Just grab the items in every selected bricks
    for (auto brickId : selection.cellSelection.insideBricks) {
        auto& brickItems = getConcreteBrick(brickId).items;
        selection.insideItems.insert(selection.insideItems.end(), brickItems.begin(), brickItems.end());

        auto& brickSubcellItems = getConcreteBrick(brickId).subcellItems;
        selection.insideSubcellItems.insert(selection.insideSubcellItems.end(), brickSubcellItems.begin(), brickSubcellItems.end());
    }

    for (auto brickId : selection.cellSelection.partialBricks) {
        auto& brickItems = getConcreteBrick(brickId).items;
        selection.partialItems.insert(selection.partialItems.end(), brickItems.begin(), brickItems.end());

        auto& brickSubcellItems = getConcreteBrick(brickId).subcellItems;
        selection.partialSubcellItems.insert(selection.partialSubcellItems.end(), brickSubcellItems.begin(), brickSubcellItems.end());
    }

    return (int) selection.numItems();
}



