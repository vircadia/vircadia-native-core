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
#include <glm/glm.hpp>
#include <glm/gtx/bit.hpp>
#include <AABox.h>

namespace render {

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

            NUM_LINKS = NUM_OCTANTS + 1,

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
        static const Depth MAX_DEPTH { 15 };
        static const double INV_DEPTH_DIM[Octree::MAX_DEPTH + 1];

        static int getDepthDimension(Depth depth) { return 2 << depth; }
        static double getInvDepthDimension(Depth depth) { return INV_DEPTH_DIM[depth]; }


        // Need 16bits integer coordinates on each axes: 32768 cell positions
        using Coord = int16_t;
        using Coord3 = glm::i16vec3;
        using Coord4 = glm::i16vec4;

        class Location {
            void assertValid() {
                assert((pos.x >= 0) && (pos.y >= 0) && (pos.z >= 0));
                assert((pos.x < (2 << depth)) && (pos.y < (2 << depth)) && (pos.z < (2 << depth)));
            }
        public:
            Location() {}
            Location(const Coord3& xyz, Depth d) : pos(xyz), depth(d) { assertValid(); }
            Location(Depth d) : pos(0), depth(d) { assertValid(); }

            Coord3 pos{ 0 };
            uint8_t spare{ 0 };
            Depth depth{ 0 };


            // Eval the octant of this cell relative to its parent
            Octant octant() const { return  Octant((pos.x & XAxis) | (pos.y & YAxis) | (pos.z & ZAxis)); }

            // Get the Parent cell Location of this cell
            Location parent() const {
                return Location{ pos >> Coord3(1), Depth(depth <= 0 ? 0 : depth - 1) };
            }
            Location child(Link octant) const {
                return Location{ pos << Coord3(1) | Coord3((Coord)bool(octant & XAxis), (Coord)bool(octant & YAxis), (Coord)bool(octant & ZAxis)), Depth(depth + 1) };
            }

            using vector = std::vector< Location >;
            static vector rootTo(const Location& dest) {
                Location current{ dest };
                vector path(dest.depth + 1);
                path[dest.depth] = dest;
                while (current.depth > 0) {
                    current = current.parent();
                    path[current.depth] = current;
                }
                return path;
            }
        };
        using CellPath = Location::vector;


        class Range {
        public:
            Coord3 _min;
            Coord3 _max;
        };

        // Cell or Brick Indexing
        using Index = int32_t;
        static const Index INVALID = -1;
        static const Index ROOT = 0;
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

            Cell() :
                _links({ { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID } })
            {}

            Cell(Index parent, Location loc) :
                _location(loc),
                _links({ { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, parent } })
            {}

            Index brick{ INVALID };

        private:
            Location _location;
            std::array<Index, NUM_LINKS> _links;
        };
        using Cells = std::vector< Cell >;


        class Brick {
        public:

        };
        using Bricks = std::vector< Brick >;



        // Octree members
        Cells _cells = Cells(1, Cell()); // start with only the Cell root
        Bricks _bricks;

        Octree() {};

        // allocatePath
        Indices allocateCellPath(const CellPath& path);

        // reach to Cell and allocate the cell path to it if needed
        Index editCell(const Location& loc) {
            auto cells = allocateCellPath(Location::rootTo(loc));
            return cells.back();
        }
    };
}
// CLose the namespace here before including the Item in the picture, maybe Octre could stay pure of it
#include "Item.h"

namespace render {

    // An octree of Items organizing them efficiently for culling
    // The octree only cares about the bound & the key of an item to store it a the right cell location
    class ItemSpatialTree : public Octree {
        float _size{ 32000.0f };
        double _invSize{ 1.0 / _size };
        glm::vec3 _origin{ -_size };
    public:


        float getSize() const { _size; }
        glm::vec3 getOrigin() const { _origin; }

        float getCellWidth(Depth depth) const { return (float) _size * getInvDepthDimension(depth); }
        float getInvCellWidth(Depth depth) const { return (float) getDepthDimension(depth) * _invSize; }

        glm::vec3 evalPos(const Coord3& coord, Depth depth = Octree::MAX_DEPTH) const {
            return getOrigin() + glm::vec3(coord) * getCellWidth(depth);
        }
        glm::vec3 evalPos(const Coord3& coord, float cellWidth) const {
            return getOrigin() + glm::vec3(coord) * cellWidth;
        }

        Coord3 evalCoord(const glm::vec3& pos, Depth depth = Octree::MAX_DEPTH) const {
            return Coord3((pos - getOrigin()) * getInvCellWidth(depth));
        }

        AABox evalBound(const Location& loc) const {
            float cellWidth = getCellWidth(loc.depth);
            return AABox(evalPos(loc.pos, cellWidth), cellWidth);
        }


        Location evalLocation(const AABox& bound) const {
            auto minPos = evalCoord(bound.getMinimumPoint());
            auto maxPos = evalCoord(bound.getMaximumPoint());
            auto range = maxPos - minPos;
            //range
                return Location(minPos, 4);
        }

        ItemSpatialTree() {}

        void insert(const ItemBounds& items);
        void erase(const ItemBounds& items);
    };
}

#endif // hifi_render_Octree_h
