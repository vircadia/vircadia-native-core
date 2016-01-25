

namespace render {
    
    class Octree {
    public:
        enum Octant {
            L_B_N = 0,
            R_B_N = 1,
            L_T_N = 2,
            R_T_N = 3,
            L_B_F = 4,
            R_B_F = 5,
            L_T_F = 6,
            R_T_F = 7,
            
            NUM_OCTANTS,
        };
        
        enum OctantBits {
            XAxis = 0x01,
            YAxis = 0x02,
            ZAXis = 0x04,
            
        };
        
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
        using Coord = int16_t// Need 16bits integer coordinates on each axes: 32768 cell positions
        using Pos = glm::vec3<Coord>;
        
        class CellCoord {
        public:
            CellCoord() {}
            CellCoord(const Pos& xyz, Depth d) : pos(xyz), depth(d) {}
            CellCoord(Depth d) : pos(0), depth(d) {}
            
            union {
                glm::vec4<Coord> raw;
                struct {
                    Pos pos { 0 };
                    int8_t spare { 0 };
                    Depth depth { 0 };
                };
            };
            
            CellCoord parent() const { return CellCoord{ pos >> 1, (d <= 0? 0 : d-1) }; }
            CellCoord child(Octant octant) const { return CellCoord{ Pos((pos.x << 1) | (octant & XAxis),
                                                                         (pos.y << 1) | (octant & YAxis),
                                                                         (pos.z << 1) | (octant & ZAxis)), d+1 }; }
        };
        
        class Range {
        public:
            Pos _min;
            Pos _max;
        }
        
        
        // the cell description
        class Cell {
        public:
            using Index = int32_t;
            static const Index INVALID = -1;

            
            Index children[NUM_OCTANTS] = { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID };
            Index parent{ INVALID };
            
            Index brick{ INVALID };
            
            CellPos cellpos;
        };
        using Cells = std::vector< Cell >;
        
        class CellPath {
        public:
            using PosArray = std::vector< CellPos >;
            PosArray cellCoords;
            
        };
        
        class Brick {
        public:
            
        }
        using Bricks = std::vector< Block >;
        
        
        
        // Octree members
        Cells _cells;
        Bricks _bricks;
        
        
        CellPath rootTo(const CellPos& dest) {
            CellPos current{ dest };
            CellPath path(dest.depth + 1)
            path[dest.depth] = dest;
            while (current.depth > 0) {
                current = current.parent();
                path[current.depth] = current);
            }
            return path;
        }
        
        CellPath rootTo(const CellPos& dest) {
            CellPos current{ dest };
            CellPath path(dest.depth + 1)
            path[dest.depth] = dest;
            while (current.depth > 0) {
                current = current.parent();
                path[current.depth] = current);
            }
            return path;
        }
    };
    
    
    
}