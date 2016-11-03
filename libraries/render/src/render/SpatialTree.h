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

#ifndef hifi_render_Octree_h
#define hifi_render_Octree_h

#include <vector>
#include <memory>
#include <cassert>
#include <array>
#include <functional>
#include <limits>
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

        void free() {};
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
        static const Depth ROOT_DEPTH { 0 };
        static const Depth MAX_DEPTH { 15 };
        static const Depth METRIC_COORD_DEPTH { 15 };
        static const float INV_DEPTH_DIM[Octree::MAX_DEPTH + 1];

        static int getDepthDimension(Depth depth) { return 1 << depth; }
        static float getDepthDimensionf(Depth depth) { return (float) getDepthDimension(depth); }
        static float getInvDepthDimension(Depth depth) { return INV_DEPTH_DIM[depth]; }
        static float getCoordSubcellWidth(Depth depth) { return (1.7320f * getInvDepthDimension(depth) * 0.5f); }

        // Need 16bits integer coordinates on each axes: 32768 cell positions
        using Coord = int16_t;
        using Coord3 = glm::i16vec3;
        using Coord4 = glm::i16vec4;
        using Coord3f = glm::vec3;
        using Coord4f = glm::vec4;

        static Coord depthBitmask(Depth depth) { return Coord(1 << (MAX_DEPTH - depth)); }

        static Depth coordToDepth(Coord length) {
            Depth depth = MAX_DEPTH;
            while (length) {
                length >>= 1;
                depth--;
            }
            return depth;
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

            Coord3 pos { 0 };
            uint8_t spare { 0 };
            Depth depth { ROOT_DEPTH };

            Coord3f getCenter() const { 
                Coord3f center = (Coord3f(pos) + Coord3f(0.5f)) * Coord3f(Octree::getInvDepthDimension(depth));
                return center;
            }

            bool operator== (const Location& other) const { return pos == other.pos && depth == other.depth; }

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
        static const Index INVALID_CELL = -1;
        static const Index ROOT_CELL = 0;

        // With a maximum of INT_MAX(2 ^ 31) cells in our octree
        static const Index MAXIMUM_INDEX = INT_MAX; // For fun, test setting this to 100 and this should still works with only the allocated maximum number of cells

        using Indices = std::vector<Index>;

        // the cell description
        class Cell {
        public:

            enum BitFlags : uint8_t {
                HasChildren = 0x01,
                BrickFilled = 0x02,
            };

            void free() { _location = Location(); for (auto& link : _links) { link = INVALID_CELL; } }

            const Location& getlocation() const { return _location; }

            Index parent() const { return _links[Parent]; }
            bool hasParent() const { return parent() != INVALID_CELL; }

            Index child(Link octant) const { return _links[octant]; }
            bool hasChild(Link octant) const { return child(octant) != INVALID_CELL; }
            bool hasChildren() const { return (_location.spare & HasChildren); }
            void setChild(Link octant, Index child) {
                _links[octant] = child;
                if (child != INVALID_CELL) {
                    _location.spare |= HasChildren;
                } else {
                    if (!checkHasChildren()) {
                        _location.spare &= ~HasChildren;
                    }
                }
            }

            Index brick() const { return _links[BrickLink]; }
            bool hasBrick() const { return _links[BrickLink] != INVALID_CELL; }
            void setBrick(Index brick) { _links[BrickLink] = brick; }

            void setBrickFilled() { _location.spare |= BrickFilled; }
            void setBrickEmpty() { _location.spare &= ~BrickFilled; }
            bool isBrickEmpty() const { return !(_location.spare & BrickFilled); }

            Cell() :
                _links({ { INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL } })
            {}

            Cell(Index parent, Location loc) :
                _links({ { INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, INVALID_CELL, parent, INVALID_CELL } }),
                _location(loc)
            {}

        private:
            std::array<Index, NUM_LINKS> _links;
            Location _location;

            bool checkHasChildren() const {
                for (LinkStorage octant = Octant_L_B_N; octant < NUM_OCTANTS; octant++) {
                    if (hasChild((Link)octant)) {
                        return true;
                    }
                }
                return false;
            }
        };
        using Cells = std::vector< Cell >;

        using Bricks = std::vector< Brick >;

        bool checkCellIndex(Index index) const { return (index >= 0) && (index < (Index) _cells.size()); }
        bool checkBrickIndex(Index index) const { return ((index >= 0) && (index < (Index) _bricks.size())); }

        Octree() {};

        // Clean a cell branch starting from the leave: 
        // Check that the cell brick is empty, if so free it else stop
        // Check that the cell has no children, if so free itself else stop
        // Apply the same logic to the parent cell
        void cleanCellBranch(Index index);

        // Indexing/Allocating the cells as the tree gets populated
        // Return the cell Index/Indices at the specified location/path, allocate all the cells on the path from the root if needed
        Indices indexCellPath(const Locations& path);
        Index indexCell(const Location& loc) { return indexCellPath(Location::pathTo(loc)).back(); }

        // Same as indexCellPath except that NO cells are allocated, only the COncrete cells previously allocated
        // the returned indices stops at the last existing cell on the requested path.
        Indices indexConcreteCellPath(const Locations& path) const;

        // Get the cell location from the CellID
        Location getCellLocation(Index cellID) const {
            if (checkCellIndex(cellID)) {
                return getConcreteCell(cellID).getlocation();
            }
            return Location();
        }

        // Reach a concrete cell
        const Cell& getConcreteCell(Index index) const {
            assert(checkCellIndex(index));
            return _cells[index];
        }

        // Let s talk about the Cell Bricks now
        using CellBrickAccessor = std::function<void(Cell& cell, Brick& brick, Index brickIdx)>;

        // acces a cell (must be concrete), then call the brick accessor if the brick exists ( or is just created if authorized to)
        // This returns the Brick index
        Index accessCellBrick(Index cellID, const CellBrickAccessor& accessor, bool createBrick = true);

        const Brick& getConcreteBrick(Index index) const {
            assert(checkBrickIndex(index));
            return _bricks[index];
        }

        // Cell Selection and traversal

        class CellSelection {
        public:
            Indices insideCells;
            Indices insideBricks;
            Indices partialCells;
            Indices partialBricks;

            Indices& cells(bool inside) { return (inside ? insideCells : partialCells); }
            Indices& bricks(bool inside) { return (inside ? insideBricks : partialBricks); }

            size_t size() const { return insideBricks.size() + partialBricks.size(); }

            void clear() {
                insideCells.clear();
                insideBricks.clear();
                partialCells.clear();
                partialBricks.clear();
            }
        };

        class FrustumSelector {
        public:
            Coord4f frustum[6];
            Coord3f eyePos;
            float   angle;
            float   squareTanAlpha;

            const float MAX_LOD_ANGLE = glm::radians(45.0f);
            const float MIN_LOD_ANGLE = glm::radians(1.0f / 60.0f);

            void setAngle(float a) {
                angle = std::max(MIN_LOD_ANGLE, std::min(MAX_LOD_ANGLE, a));
                auto tanAlpha = tan(angle);
                squareTanAlpha = (float)(tanAlpha * tanAlpha);
            }

            float testSolidAngle(const Coord3f& point, float size) const {
                auto eyeToPoint = point - eyePos;
                return (size * size / glm::dot(eyeToPoint, eyeToPoint)) - squareTanAlpha;
            }
        };

        int select(CellSelection& selection, const FrustumSelector& selector) const;
        int selectTraverse(Index cellID, CellSelection& selection, const FrustumSelector& selector) const;
        int selectBranch(Index cellID, CellSelection& selection, const FrustumSelector& selector) const;
        int selectCellBrick(Index cellID, CellSelection& selection, bool inside) const;


        int getNumAllocatedCells() const { return (int)_cells.size(); }
        int getNumFreeCells() const { return (int)_freeCells.size(); }
            
    protected:
        Index allocateCell(Index parent, const Location& location);
        void freeCell(Index index);

        Index allocateBrick();
        void freeBrick(Index index);

        Cell& editCell(Index index) {
            assert(checkCellIndex(index));
            return _cells[index];
        }


        // Octree members
        Cells _cells = Cells(1, Cell()); // start with only the Cell root
        Bricks _bricks;
        Indices _freeCells; // stack of free cells to be reused for allocation
        Indices _freeBricks; // stack of free bricks to be reused for allocation
    };
}

// CLose the namespace here before including the Item in the picture, maybe Octre could stay pure of it
namespace render {

    // An octree of Items organizing them efficiently for culling
    // The octree only cares about the bound & the key of an item to store it a the right cell location
    class ItemSpatialTree : public Octree {
        float _size{ 32768.0f };
        float _invSize { 1.0f / _size };
        glm::vec3 _origin { -16384.0f };

        void init(glm::vec3 origin, float size) {
            _size = size;
            _invSize = 1.0f / _size;
            _origin = origin;
        }
    public:
        // THe overall size and origin of the tree are defined at creation
        ItemSpatialTree(glm::vec3 origin, float size) { init(origin, size); }

        float getSize() const { return _size; }
        const glm::vec3& getOrigin() const { return _origin; }

        float getCellWidth(Depth depth) const { return _size * getInvDepthDimension(depth); }
        float getInvCellWidth(Depth depth) const { return getDepthDimensionf(depth) * _invSize; }

        float getCellHalfDiagonalSquare(Depth depth) const {
            float cellHalfWidth = 0.5f * getCellWidth(depth);
            return 3.0f * cellHalfWidth * cellHalfWidth;
        }

        glm::vec3 evalPos(const Coord3& coord, Depth depth = Octree::METRIC_COORD_DEPTH) const {
            return getOrigin() + glm::vec3(coord) * getCellWidth(depth);
        }
        glm::vec3 evalPos(const Coord3& coord, float cellWidth) const {
            return getOrigin() + glm::vec3(coord) * cellWidth;
        }


        // Clamp a 3D relative position to make sure it is in the valid range space of the octree
        glm::vec3 clampRelPosToTreeRange(const glm::vec3& pos) const {
            const float EPSILON = 0.0001f;
            return glm::vec3(
                std::min(std::max(pos.x, 0.0f), _size - EPSILON),
                std::min(std::max(pos.y, 0.0f), _size - EPSILON),
                std::min(std::max(pos.z, 0.0f), _size - EPSILON));
        }

        // Eval an integer cell coordinate (at the specified deepth) from a given 3d position
        // If the 3D position is out of the octree volume, then the position is clamped
        // so the integer coordinate is meaningfull
        Coord3 evalCoord(const glm::vec3& pos, Depth depth = Octree::METRIC_COORD_DEPTH) const {
            auto npos = clampRelPosToTreeRange((pos - getOrigin()));
            return Coord3(npos * getInvCellWidth(depth)); // Truncate fractional part
        }

        // Eval a real cell coordinate (at the specified deepth) from a given 3d position
        // Position is NOT clamped to the boundaries of the octree so beware of conversion to a Coord3!
        Coord3f evalCoordf(const glm::vec3& pos, Depth depth = Octree::METRIC_COORD_DEPTH) const {
            auto npos = (pos - getOrigin());
            return Coord3f(npos * getInvCellWidth(depth));
        }

        // Bound to Location
        AABox evalBound(const Location& loc) const {
            float cellWidth = getCellWidth(loc.depth);
            return AABox(evalPos(loc.pos, cellWidth), cellWidth);
        }

        // Eval the cell location for a given arbitrary Bound,
        // if the Bound crosses any of the Octree planes then the root cell is returned
        Location evalLocation(const AABox& bound, Coord3f& minCoordf, Coord3f& maxCoordf) const;
        Locations evalLocations(const ItemBounds& bounds) const;

        // Managing itemsInserting items in cells
        // Cells need to have been allocated first calling indexCell
        Index insertItem(Index cellIdx, const ItemKey& key, const ItemID& item);
        bool updateItem(Index cellIdx, const ItemKey& oldKey, const ItemKey& key, const ItemID& item);
        bool removeItem(Index cellIdx, const ItemKey& key, const ItemID& item);

        Index resetItem(Index oldCell, const ItemKey& oldKey, const AABox& bound, const ItemID& item, ItemKey& newKey);

        // Selection and traverse
        int selectCells(CellSelection& selection, const ViewFrustum& frustum, float lodAngle) const;

        class ItemSelection {
        public:
            CellSelection cellSelection;
            ItemIDs insideItems;
            ItemIDs insideSubcellItems;
            ItemIDs partialItems;
            ItemIDs partialSubcellItems;

            ItemIDs& items(bool inside) { return (inside ? insideItems : partialItems); }
            ItemIDs& subcellItems(bool inside) { return (inside ? insideSubcellItems : partialSubcellItems); }

            size_t insideNumItems() const { return insideItems.size() + insideSubcellItems.size(); }
            size_t partialNumItems() const { return partialItems.size() + partialSubcellItems.size(); }
            size_t numItems() const { return insideNumItems() + partialNumItems(); }

            void clear() {
                cellSelection.clear();
                insideItems.clear();
                insideSubcellItems.clear();
                partialItems.clear();
                partialSubcellItems.clear();
            }
        };

        int selectCellItems(ItemSelection& selection, const ItemFilter& filter, const ViewFrustum& frustum, float lodAngle) const;
    };
}

#endif // hifi_render_Octree_h
