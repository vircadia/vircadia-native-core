/*
 Let s try compressing images
 
*/

#include <glm/glm.hpp>
#include "Forward.h"


namespace image {

    // Storage types
    using Byte = uint8_t;
    using Byte2 = uint16_t;
    using Byte4 = uint32_t;
    using Byte8 = uint64_t;
    
    // Storage type selector based on size (sizeof of a type)
    template<int size, typename T1 = Byte, typename T2 = Byte2, typename T4 = Byte4, typename T8 = Byte8>
    struct storage { typedef T1 type; };
    
    template<typename T1, typename T2, typename T4, typename T8>
    struct storage<2, T1, T2, T4, T8> { typedef T2 type; };
    
    template<typename T1, typename T2, typename T4, typename T8>
    struct storage<4, T1, T2, T4, T8> { typedef T4 type; };
    
    template<typename T1, typename T2, typename T4, typename T8>
    struct storage<8, T1, T2, T4, T8> { typedef T8 type; };
    
    static const Byte BLACK8 { 0 };
    static const Byte WHITE8 { 255 };
    
    template <int N> int bitVal() { return 1 << N; }
    template <int Tn, int An> int bitProduct() { return bitVal<Tn>() * bitVal<An>(); }
    template <int Tn, int An, typename T = Byte, typename A = T> T mix(const T x, const T y, const A a) { return T(((bitVal<An>() - a) * x + a * y) / bitProduct<Tn, An>()); }
    
    Byte mix5_4(const Byte x, const Byte y, const Byte a) { return mix<5, 4>(x, y, a); }
    Byte mix6_4(const Byte x, const Byte y, const Byte a) { return mix<6, 4>(x, y, a); }
    
    
    namespace pixel {
        
        struct RGB32 {
            Byte r { BLACK8 };
            Byte g { BLACK8 };
            Byte b { BLACK8 };
            Byte x { WHITE8 };
        };
        
        struct RGBA32 {
            Byte r { BLACK8 };
            Byte g { BLACK8 };
            Byte b { BLACK8 };
            Byte a { WHITE8 };
        };

        struct R10G10B12 {
            Byte4 r : 10;
            Byte4 g : 10;
            Byte4 b : 12;
        };
        
        struct RGB16_565 {
            Byte2 b : 5;
            Byte2 g : 6;
            Byte2 r : 5;
            
            RGB16_565() : b(BLACK8), g(BLACK8), r(BLACK8) {}
            RGB16_565(Byte pR, Byte pG, Byte pB) : b(pB), g(pG), r(pR) {}
        };
        
        struct R8 {
            Byte r { BLACK8 };
        };
        

        template <typename P, typename S> const P mix(const P p0, const P p1, const S alpha) { return p0; }
        template <> const RGB16_565 mix(const RGB16_565 p0, const RGB16_565 p1, const Byte alpha);

        template <typename F, typename S = typename storage<sizeof(F)>::type > class Pixel {
        public:
            using Format = F;
            using Storage = S;
            
            union {
                Storage raw;
                Format val{ Format() }; // Format last to be initialized by Format's default constructor
            };
            
            Pixel() {};
            Pixel(Format v) : val(v) {}
            Pixel(Storage s) : raw(s) {}
        };

        
        template <typename P, int length> class PixelBlock {
        public:
            using Format = typename P::Format;
            using Storage = typename P::Storage;
            
            constexpr uint16_t getLength() const { return length; }
            uint32_t getSize() const { return length * sizeof(P); }
            
            P pixels[length];
            
            PixelBlock() {}
            
            
            PixelBlock(const P* srcPixels) {
                setPixels(srcPixels);
            }
            
            void setPixels(const P* srcPixels) {
                memcpy(pixels, srcPixels, getSize());
            }
            
            const Storage* getStorage() const { return static_cast<const Storage*> (&pixels->raw); }
        };
        
        template <typename P, int tileW, int tileH> class Tile {
        public:
            using Format = typename P::Format;
            using Storage = typename P::Storage;
            using Block = typename PixelBlock<P, tileW * tileH>;
            
            constexpr uint16_t getWidth() const { return tileW; }
            constexpr uint16_t getHeight() const { return tileH; }
            
            Block _block;
            
            Tile() {}
            Tile(const P* srcPixels) : _block(srcPixels) {}
            
            
        };
    };



    using PixRGB565 = pixel::Pixel<pixel::RGB16_565>;
    using PixRGB32 = pixel::Pixel<pixel::RGB32>;
    using PixRGBA32 = pixel::Pixel<pixel::RGBA32>;
    using PixR10G10B12 = pixel::Pixel<pixel::R10G10B12>;
    using PixR8 = pixel::Pixel<pixel::R8>;
    
    
    class BC {
    public:
        static int cpp;
        
        struct BC1 {
            PixRGB565 color0;
            PixRGB565 color1;
            Byte4 table;
        };
        
        struct BC4 {
            PixRGB565 color0;
            PixRGB565 color1;
            Byte4 table;
        };
    };
    
    template <typename T> class CompressedBlock {
    public:
        static const uint32_t SIZE { sizeof(T) };
        union {
            Byte bytes[SIZE];
            T bc;
        };
        
        CompressedBlock() {}
    };
    
    
    template <typename PB, typename CB> void compress(const PB& srcBlock, CB& dstBlock) { }
    template <typename PB, typename CB> void uncompress(const CB& srcBlock, PB& dstBlock) { }


    using PB_RGB32 = pixel::PixelBlock<PixRGB32, 16>;
    using PB_RGBA32 = pixel::PixelBlock<PixRGBA32, 16>;
    
    using CB_BC1 = CompressedBlock<BC::BC1>;
    using CB_BC4 = CompressedBlock<BC::BC4>;
    
    template <> void compress(const PB_RGB32& src, CB_BC1& dst);
    template <> void compress(const PB_RGBA32& src, CB_BC4& dst);

    template <> void uncompress(const CB_BC1& src, PB_RGB32& dst);
    template <> void uncompress(const CB_BC4& src, PB_RGBA32& dst);
    
    
    template <typename B>
    class PixelBlockArray {
    public:
        using Block = B;
        
        using Blocks = std::vector<Block>;
        
        Blocks _blocks;
    };
    
    class Grid {
    public:
        using Coord = uint16_t;
        using Coord2 = glm::u16vec2;
        using Size = uint32_t;
        
        static const Coord2 TILE_PIXEL;
        static const Coord2 TILE_QUAD;
        static const Coord2 TILE_DEFAULT;
        
        Grid(const Coord2& surface, const Coord2& tile = TILE_DEFAULT) : _surface(surface), _tile(tile) {}
        Grid(Coord width, Coord height, const Coord2& tile = TILE_DEFAULT) : _surface(width, height), _tile(tile) {}
        
        Coord2 _surface { 1, 1 };
        Coord2 _tile { TILE_DEFAULT };
        
        Coord width() const { return _surface.x; }
        Coord height() const { return _surface.y; }
        const Coord2& size() const { return _surface; }

        Coord tileWidth() const { return evalNumTiles(_surface.x, _tile.x); }
        Coord tileHeight() const { return evalNumTiles(_surface.y, _tile.y); }
        Coord2 tileSize() const { return Coord2(tileWidth(), tileHeight()); }
    
    
        Coord2 toTile(const Coord2& pixel) const { return pixel / _tile; }
        Coord2 toTileSubpix(const Coord2& pixel) const { return pixel % _tile; }
        Coord2 toTile(const Coord2& pixel, Coord2& subpix) const {
            subpix = toTileSubpix(pixel);
            return toTile(pixel);
        }
        
        Coord2 toPixel(const Coord2& tile) const { return tile * _tile; }
        Coord2 toPixel(const Coord2& tile, const Coord2& subpix) const { return tile * _tile + subpix; }
        
        
        static Coord evalNumTiles(Coord pixelLength, Coord tileLength) {
            auto tilePos = pixelLength / tileLength;
            if (tilePos * tileLength < pixelLength) {
                tilePos++;
            }
            return tilePos;
        }
    };
    
    template <typename T>
    class Pixmap {
    public:
        using Tile = T;
        
        Grid _grid;
        PixelBlockArray<T::Block> _blocks;
        
        void resize(const Grid::Coord2& widthHeight) {
            _grid = Grid(widthHeight, Coord2(Tile::getWidth(), Tile::getHeight()));
            
        }
       
    };
}