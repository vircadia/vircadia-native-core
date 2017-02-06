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



    // Coordinate and count types
    using Coord = uint16_t;
    using Coord2 = glm::u16vec2;
    using Count = uint32_t;
    using Index = uint32_t;

    // Maximum pixel along one direction coord is 32768 
    static const Coord MAX_COORD { 32768 };
    // Maximum number of pixels in an image
    static const Count MAX_COUNT { MAX_COORD * MAX_COORD };
    static const Index MAX_INDEX { MAX_COUNT };


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
            using This = Pixel<F,S>;
            using Format = F;
            using Storage = S;

            static const uint32_t SIZE { sizeof(S) };

            Format val { Format() };

            Pixel() : val(Format()) {};
            Pixel(Format v) : val(v) {}
            Pixel(Storage s) : val(*static_cast<Format> (&s)) {}

            const Storage* storage() const { return static_cast<Storage> (&val); }
            
            static This* cast(Storage* s) { return reinterpret_cast<This*>(s); }
            static const This* cast(const Storage* s) { return reinterpret_cast<const This*>(s); }
        };

        template <typename P, int length> class PixelBlock {
        public:
            using Format = typename P::Format;
            using Storage = typename P::Storage;

            static const uint16_t LENGTH { length };
            static const uint32_t SIZE { length * sizeof(P) };

            P pixels[length];

            PixelBlock() {}

            PixelBlock(const P* srcPixels) {
                setPixels(srcPixels);
            }

            void setPixels(const P* srcPixels) {
                memcpy(pixels, srcPixels, SIZE);
            }

            const Storage* storage() const { return pixels->storage(); }
        };
        
        template <typename P, int tileW, int tileH> class Tile {
        public:
            using Format = typename P::Format;
            using Storage = typename P::Storage;

            using Block = PixelBlock<P, tileW * tileH>;

            uint16_t getWidth() const { return tileW; }
            uint16_t getHeight() const { return tileH; }

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
    
    template <typename F, typename S = typename storage<sizeof(F)>::type> class CompressedBlock {
    public:
        using Format = F;
        using Storage = S;

        static const uint32_t SIZE { sizeof(F) };

        Format bc;

        CompressedBlock() {}

        const Storage* storage() const { return static_cast<Storage> (&bc); }
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
    
    
    template <typename P>
    class PixelArray {
    public:
        using This = PixelArray<P>;
        using Pixel = P;
        using Storage = typename P::Storage;
        
        static int evalNumPixels(size_t byteSize) {
            size_t numPixels = byteSize / Pixel::SIZE;
            if (byteSize > numPixels * Pixel::SIZE) {
                numPixels++;
            }
            return (int)numPixels;
        };
        static size_t evalByteSize(int numPixels) {
            return numPixels * Pixel::SIZE;
        };
        
        struct Storages {
            Storage* _bytes {nullptr};
            Count _count {0};
            
            Storage* data() {
                return _bytes;
            }
            const Storage* data() const {
                return _bytes;
            }
            
            Count count() const { return _count; }
            
            ~Storages() { if (_bytes) { delete _bytes; } }
            Storages() {}
            Storages(Count c) : _count(c) { if (_count) { _bytes = new Storage[_count]; }
                
            }
        };

        

        PixelArray() {
            resetBytes(0, nullptr);
        }
        PixelArray(size_t byteSize, const void* bytes) {
            resetBytes(byteSize, bytes);
        }
        PixelArray(const PixelArray& src) {
            resetBytes(src.byteSize(), src.readBytes(0));
        }
        PixelArray(PixelArray&& src) {
            _buffer.reset(src._buffer.release());
        }
        This& operator = (This&& src) {
            _buffer.reset(src._buffer.release());
        }
    
        int numPixels() const { return _buffer->count(); }
        size_t byteSize() const { return evalByteSize(_buffer->count()); }

        //access storage or pixel types at index in the buffer
        const Storage* readBytes(const Index index) const { return _buffer->data() + index; }
        const Pixel* readPixel(const Index index) const { return Pixel::cast(readBytes(index)); }
        
        Storage* editBytes(const Index index) { return _buffer->data() + index; }
        Pixel* editPixel(const Index index) { return Pixel::cast(editBytes(index)); }
    
    private:
        std::unique_ptr<Storages> _buffer;

        void resetBytes(size_t byteSize, const void* bytes) {
            _buffer.reset(new Storages(evalNumPixels(byteSize)));
            if (bytes) {
                memcpy(_buffer->data(), bytes, byteSize);
            }
        }
    };

    template <typename B>
    class PixelBlockArray {
    public:
        using Block = B;
        using Blocks = std::vector<Block>;

        static int evalNumBlocks(size_t byteSize) {
            size_t numBlocks = byteSize / Block::SIZE;
            if (byteSize > numBlocks * Block::SIZE) {
                numBlocks++;
            }
            return (int) numBlocks;
        };
        static size_t evalByteSize(int numBlocks) {
            return numBlocks * Block::SIZE;
        };

        PixelBlockArray(size_t byteSize, const void* bytes) {
            setBytes(byteSize, bytes);
        }

        int numBlocks() const { return evalByteSize(_blocks.size()); }
        size_t byteSize() const { return evalByteSize(_blocks.size()); }

        Blocks _blocks;

    private:
        void setBytes(size_t byteSize, const void* bytes) {
            _blocks = Blocks(evalNumBlocks(byteSize));
            if (bytes) {
                memcpy(_blocks.data(), bytes, byteSize);
            }
        }
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
    class Tilemap {
    public:
        using Tile = T;
        using Block = typename T::Block;

        Grid _grid;
        PixelBlockArray<Block> _blocks;
        
        void resize(const Grid::Coord2& widthHeight) {
            _grid = Grid(widthHeight, Coord2(Tile::getWidth(), Tile::getHeight()));
            
        }
       
    };

    class Dim {
    public:

        Coord2 _dims { 0 };

        static Coord cap(Coord c) { return (c < MAX_COORD ? c : MAX_COORD); }
        static Coord2 cap(const Coord2& dims) { return Coord2(cap(dims.x), cap(dims.y)); }

        static Count numPixels(const Coord2& dims) { return Count(cap(dims.x)) * Count(cap(dims.y)); }


        static Coord nextMip(Coord c) { return (c > 1 ? (c >> 1) : c); }
        static Coord2 nextMip(const Coord2& dims) { return Coord2(nextMip(dims.x), nextMip(dims.y)); }

        Dim(Coord w, Coord h) : _dims(w, h) {}
        Dim(const Coord2& dims) : _dims(dims) {}

        Count numPixels() const { return Dim::numPixels(_dims); }
        Dim nextMip() const { return Dim(nextMip(_dims)); }

        int maxLevel() const {
            int level = 0;
            auto dim = (*this);
            while (dim._dims.x > 1 || dim._dims.y > 1) {
                level++;
                dim = dim.nextMip();
            }
            return level;
        }
        
        Index indexAt(Coord x, Coord y) const { return (Index) y * (Index) _dims.x + (Index) x; }
    };

    template < typename P > class Surface {
    public:
        using This = Surface<P>;
        using Pixel = P;
        using Format = typename P::Format;
        using Pixels = PixelArray<P>;

        Dim _dim { 0, 0 };
        Pixels _pixels;

      /*  Surface(This&& src) {
            _dim = src._dim;
            _pixels = src._pixels;
            
        }*/
        Surface(Coord width, Coord height, size_t byteSize = 0, const void* bytes = nullptr) :
            _dim(width, height),
            _pixels((byteSize ? byteSize : Pixels::evalByteSize(width * height)), bytes)
        {}

        This nextMip() const {
            Dim subDim = _dim.nextMip();
            auto sub = Surface<P>(subDim._dims.x, subDim._dims.y);
            
            for (int y = 0; y < subDim._dims.y; y++) {
                for (int x = 0; x < subDim._dims.x; x++) {
                    
                    // get pixels from source at 2x, 2x +1 2y, 2y +1
                    auto srcIndex0 = _dim.indexAt(2 * x, 2 * y);
                    auto srcIndex1 = _dim.indexAt(2 * x, 2 * y + 1);
                    
                    auto srcP0 = (*_pixels.readPixel(srcIndex0));
                    auto srcP01 = (*_pixels.readPixel(srcIndex0 + 1));
                    auto srcP1 = (*_pixels.readPixel(srcIndex1));
                    auto srcP11 = (*_pixels.readPixel(srcIndex1 + 1));
                    
                    // and filter
                    auto destIndex = subDim.indexAt(x, y);
                    auto destP = sub._pixels.editPixel(destIndex);
                    
                    // assign to dest
                    *destP = srcP0;
                }
            }
            return sub;
        }
        
        void downsample(std::vector<This>& mips, int num) const {
            if (num == 0) {
                return;
            }
            auto maxLevel = _dim.maxLevel();
        
            if (num == -1 || num > maxLevel) {
                num = maxLevel;
            }
            else if (num < -1) {
                return;
            }
        
            
            mips.emplace_back(nextMip());
        
            for (int i = 1; i < num; i++) {
                mips.emplace_back(mips.back().nextMip());
            }
            
        }

    };
}

