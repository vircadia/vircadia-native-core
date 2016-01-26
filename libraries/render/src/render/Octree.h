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


        // Depth, Width, Volume
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
        
        using Depth = int8_t; // Max depth is 15 => 32Km root down to 1m cells
        using Coord = int16_t;// Need 16bits integer coordinates on each axes: 32768 cell positions
        using Coord3 = glm::i16vec3;
        using Coord4 = glm::i16vec4;
        
        class CellPoint {
            void assertValid() {
                assert((pos.x >= 0) && (pos.y >= 0) && (pos.z >= 0));
                assert((pos.x < (2 << depth)) && (pos.y < (2 << depth)) && (pos.z < (2 << depth)));
            }
        public:
            CellPoint() {}
            CellPoint(const Coord3& xyz, Depth d) : pos(xyz), depth(d) { assertValid(); }
            CellPoint(Depth d) : pos(0), depth(d) { assertValid(); }

            Coord3 pos{ 0 };
            uint8_t spare{ 0 };
            Depth depth{ 0 };


            // Eval the octant of this cell relative to its parent
            Octant octant() const { return  Octant((pos.x & XAxis) | (pos.y & YAxis) | (pos.z & ZAxis)); }

            // Get the Parent cell pos of this cell
            CellPoint parent() const {
                return CellPoint{ pos >> Coord3(1), Depth(depth <= 0 ? 0 : depth - 1) };
            }
            CellPoint child(Link octant) const {
                return CellPoint{ pos << Coord3(1) | Coord3((Coord)bool(octant & XAxis), (Coord)bool(octant & YAxis), (Coord)bool(octant & ZAxis)), Depth(depth + 1) };
            }

            using vector = std::vector< CellPoint >;
            static vector rootTo(const CellPoint& dest) {
                CellPoint current{ dest };
                vector path(dest.depth + 1);
                path[dest.depth] = dest;
                while (current.depth > 0) {
                    current = current.parent();
                    path[current.depth] = current;
                }
                return path;
            }
        };
        using CellPath = CellPoint::vector;

        
        class Range {
        public:
            Coord3 _min;
            Coord3 _max;
        };

        // Cell or Brick Indexing
        using Index = int32_t;
        static const Index INVALID = -1;
        using Indices = std::vector<Index>;

        // the cell description
        class Cell {
        public:

            CellPoint cellpos;

            std::array<Index, NUM_LINKS> links;

            Index parent() const { return links[Parent]; }
            bool asParent() const { return parent() != INVALID; }

            Index child(Link octant) const { return links[octant]; }
            bool asChild(Link octant) const { return child(octant) != INVALID; }

            Index brick{ INVALID };

            Cell() :
                links({ { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID } })
            {}
            
            Cell(Index parent, CellPoint pos) :
                cellpos(pos),
                links({ { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, parent } })
            {}
        };
        using Cells = std::vector< Cell >;
        

        class Brick {
        public:
            
        };
        using Bricks = std::vector< Brick >;
        
        
        
        // Octree members
        Cells _cells = Cells(1, Cell()); // start with only the Cell root
        Bricks _bricks;
        
        float _size = 320.0f;
        
        Octree() {};

        // allocatePath
        Indices allocateCellPath(const CellPath& path);

        AABox evalBound(const CellPoint& point) const {
            
            float width = (float) (_size / double(1 << point.depth));
            glm::vec3 corner = glm::vec3(-_size * 0.5f) + glm::vec3(point.pos) * width;
            return AABox(corner, width);
            
        }
    };
    
    
    
}

#endif // hifi_render_Octree_h
