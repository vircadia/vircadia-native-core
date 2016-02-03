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

#ifndef hifi_render_Octree_h
#define hifi_render_Octree_h

#include <vector>
#include <memory>
#include <cassert>
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtx/bit.hpp>
#include <AABox.h>

// maybe we could avoid the Item inclusion here for the OCtree class?
#include "Item.h"

namespace render {

    class Brick {
    public:
        std::vector<ItemID> items;
        std::vector<ItemID> subcellItems;
    };

    class Octree {
    public:

        using LinkStorage = int8_t;
        enum Link : LinkStorage {

            Octant_L_B_N = 0,
            Octant_R_B_N = 1,
            Octant_L_T_N = 2,
            Octant_R_T_N = 3,
            Octant_L_B_F = 4,
            Octant_R_B_F = 5,
            Octant_L_T_F = 6,
            Octant_R_T_F = 7,

            NUM_OCTANTS,

            Parent = NUM_OCTANTS,
            BrickLink = Parent + 1,

            NUM_LINKS = BrickLink + 1,


            XAxis = 0x01,
            YAxis = 0x02,
            ZAxis = 0x04,

            NoLink = 0x0F,
        };

        using Octant = Link;


        // Depth, Dim, Volume
        // {0,  1,     1}
        // {1,  2,     8}
        // {2,  4,     64}
        // {3,  8,     512}
        // {4,  16,    4096}
        // {5,  32,    32768}
        // {6,  64,    262144}
        // {7,  128,   2097152}
        // {8,  256,   16777216}
        // {9,  512,   134217728}
        // {10, 1024,  1073741824}
        // {11, 2048,  8589934592}
        // {12, 4096,  68719476736}
        // {13, 8192,  549755813888}
        // {14, 16384, 4398046511104}
        // {15, 32768, 35184372088832}
        // {16, 65536, 281474976710656}

        // Max depth is 15 => 32Km root down to 1m cells
        using Depth = int8_t;
        static const Depth ROOT_DEPTH{ 0 };
        static const Depth MAX_DEPTH { 15 };
        static const double INV_DEPTH_DIM[Octree::MAX_DEPTH + 1];

        static int getDepthDimension(Depth depth) { return 1 << depth; }
        static double getInvDepthDimension(Depth depth) { return INV_DEPTH_DIM[depth]; }

        // Need 16bits integer coordinates on each axes: 32768 cell positions
        using Coord = int16_t;
        using Coord3 = glm::i16vec3;
        using Coord4 = glm::i16vec4;
        using Coord3f = glm::vec3;
        using Coord4f = glm::vec4;

        static Coord depthBitmask(Depth depth) { return Coord(1 << (MAX_DEPTH - depth)); }

        static Depth coordToDepth(Coord length) {
            Depth d = MAX_DEPTH;
            while (length) {
                length >>= 1;
                d--;
            }
            return d;
        }


        class Location {
            void assertValid() {
                assert((pos.x >= 0) && (pos.y >= 0) && (pos.z >= 0));
                assert((pos.x < (1 << depth)) && (pos.y < (1 << depth)) && (pos.z < (1 << depth)));
            }
        public:
            using vector = std::vector< Location >;

            Location() {}
            Location(const Coord3& xyz, Depth d) : pos(xyz), depth(d) { assertValid(); }
            Location(Depth d) : pos(0), depth(d) { assertValid(); }

            Coord3 pos{ 0 };
            uint8_t spare{ 0 };
            Depth depth{ ROOT_DEPTH };

            bool operator== (const Location& right) const { return pos == right.pos && depth == right.depth; }

            // Eval the octant of this cell relative to its parent
            Octant octant() const { return  Octant((pos.x & 1) | ((pos.y & 1) << 1) | ((pos.z & 1) << 2)); }
            Coord3 octantAxes(Link octant) const { return Coord3((Coord)bool(octant & XAxis), (Coord)bool(octant & YAxis), (Coord)bool(octant & ZAxis)); }

            // Get the Parent cell Location of this cell
            Location parent() const { return Location{ (pos >> Coord3(1)), Depth(depth <= 0 ? 0 : depth - 1) }; }
            Location child(Link octant) const { return Location{ (pos << Coord3(1)) | octantAxes(octant), Depth(depth + 1) }; }

            // Eval the list of locations (the path) from root to the destination.
            // the root cell is included
            static vector pathTo(const Location& destination);

            // Eval the location best fitting the specified range
            static Location evalFromRange(const Coord3& minCoord, const Coord3& maxCoord, Depth rangeDepth = MAX_DEPTH);

            // Eval the intersection test against a frustum
            enum Intersection {
                Outside = 0,
                Intersect,
                Inside,
            };
            static Intersection intersectCell(const Location& cell, const Coord4f frustum[6]);

        };
        using Locations = Location::vector;

        // Cell or Brick Indexing
        using Index = ItemCell; // int32_t
        static const Index INVALID = -1;
        static const Index ROOT_CELL = 0;
        using Indices = std::vector<Index>;

        // the cell description
        class Cell {
        public:
            const Location& getlocation() const { return _location; }

            Index parent() const { return _links[Parent]; }
            bool asParent() const { return parent() != INVALID; }

            Index child(Link octant) const { return _links[octant]; }
            bool asChild(Link octant) const { return child(octant) != INVALID; }
            void setChild(Link octant, Index child) { _links[octant] = child; }

            Index brick() const { return _links[BrickLink]; }
            bool hasBrick() const { return _links[BrickLink] != INVALID; }
            void setBrick(Index brick) { _links[BrickLink] = brick; }
            void signalBrickFilled() { _location.spare = 1; }
            void signalBrickEmpty() { _location.spare = 0; }
            bool isBrickEmpty() const { return _location.spare == 0; }

            Cell() :
                _links({ { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID } })
            {}

            Cell(Index parent, Location loc) :
                _location(loc),
                _links({ { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, parent, INVALID } })
            {}

        private:
            std::array<Index, NUM_LINKS> _links;
            Location _location;
        };
        using Cells = std::vector< Cell >;

        using Bricks = std::vector< Brick >;


        // Octree members
        Cells _cells = Cells(1, Cell()); // start with only the Cell root
        Bricks _bricks;

        Octree() {};

        // Indexing/Allocating the cells as the tree gets populated
        // Return the cell Index/Indices at the specified location/path, allocate all the cells on the path from the root if needed
        Indices indexCellPath(const Locations& path);
        Index indexCell(const Location& loc) { return indexCellPath(Location::pathTo(loc)).back(); }

        // Same as indexCellPath except that NO cells are allocated, only the COncrete cells previously allocated
        // the returned indices stops at the last existing cell on the requested path.
        Indices indexConcreteCellPath(const Locations& path) const;

        // Get the cell location from the CellID
        Location getCellLocation(Index cellID) const {
            if ((cellID >= 0) && (cellID < _cells.size())) {
                return getConcreteCell(cellID).getlocation();
            }
            return Location();
        }

        // Reach a concrete cell
        const Cell& getConcreteCell(Index index) const {
            assert(index < _cells.size());
            return _cells[index];
        }



        // Let s talk about the Cell Bricks now
        using CellBrickAccessor = std::function<void(Cell& cell, Brick& brick, Index brickIdx)>;

        // acces a cell (must be concrete), then call the brick accessor if the brick exists ( or is just created if authorized to)
        // This returns the Brick index
        Index accessCellBrick(Index cellID, const CellBrickAccessor& accessor, bool createBrick = true);


        const Brick& getConcreteBrick(Index index) const {
            assert(index < _bricks.size());
            return _bricks[index];
        }

        // Selection and traverse
        int select(Indices& selectedBricks, Indices& selectedCells, const Coord4f frustum[6]) const;
        int selectTraverse(Index cellID, Indices& selectedBricks, Indices& selectedCells, const Coord4f frustum[6]) const;
        int selectBranch(Index cellID, Indices& selectedBricks, Indices& selectedCells, const Coord4f frustum[6]) const;
        int selectInCell(Index cellID, Indices& selectedBricks, Indices& selectedCells, const Coord4f frustum[6]) const;

    protected:
        Index allocateCell(Index parent, const Location& location);
        Index allocateBrick();

        Cell& editCell(Index index) {
            assert(index < _cells.size());
            return _cells[index];
        }

    };
}

// CLose the namespace here before including the Item in the picture, maybe Octre could stay pure of it
namespace render {

    // An octree of Items organizing them efficiently for culling
    // The octree only cares about the bound & the key of an item to store it a the right cell location
    class ItemSpatialTree : public Octree {
        float _size{ 32768.0f };
        double _invSize{ 1.0 / _size };
        glm::vec3 _origin{ -16384.0f };
    public:
        ItemSpatialTree() {}

        float getSize() const { return _size; }
        const glm::vec3& getOrigin() const { return _origin; }

        float getCellWidth(Depth depth) const { return (float) _size * getInvDepthDimension(depth); }
        float getInvCellWidth(Depth depth) const { return (float) getDepthDimension(depth) * _invSize; }

        glm::vec3 evalPos(const Coord3& coord, Depth depth = Octree::MAX_DEPTH) const {
            return getOrigin() + glm::vec3(coord) * getCellWidth(depth);
        }
        glm::vec3 evalPos(const Coord3& coord, float cellWidth) const {
            return getOrigin() + glm::vec3(coord) * cellWidth;
        }

        Coord3 evalCoord(const glm::vec3& pos, Depth depth = Octree::MAX_DEPTH) const {
            auto npos = (pos - getOrigin());
            return Coord3(npos * getInvCellWidth(depth)); // Truncate fractional part
        }
        Coord3f evalCoordf(const glm::vec3& pos, Depth depth = Octree::MAX_DEPTH) const {
            auto npos = (pos - getOrigin());
            return Coord3f(npos * getInvCellWidth(depth));
        }

        // Bound to Location
        AABox evalBound(const Location& loc) const {
            float cellWidth = getCellWidth(loc.depth);
            return AABox(evalPos(loc.pos, cellWidth), cellWidth);
        }
        Location evalLocation(const AABox& bound) const {
            return Location::evalFromRange(evalCoord(bound.getMinimumPoint()), evalCoord(bound.getMaximumPoint()));
        }
        Locations evalLocations(const ItemBounds& bounds) const;

        // Managing itemsInserting items in cells
        // Cells need to have been allocated first calling indexCell
        Index insertItem(Index cellIdx, const ItemKey& key, const ItemID& item);
        bool updateItem(Index cellIdx, const ItemKey& oldKey, const ItemKey& key, const ItemID& item);
        bool removeItem(Index cellIdx, const ItemKey& key, const ItemID& item);

        Index resetItem(Index oldCell, const ItemKey& oldKey, const AABox& bound, const ItemID& item, ItemKey& newKey);

        // Selection and traverse
        int select(Indices& selectedBricks, Indices& selectedCells, const ViewFrustum& frustum) const;
        int fetch(ItemIDs& fetchedItems, const ItemFilter& filter, const ViewFrustum& frustum) const;
    };
}

#endif // hifi_render_Octree_h
