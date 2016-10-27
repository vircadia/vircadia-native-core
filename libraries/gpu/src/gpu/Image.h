/*
 Let s try compressing images
 
*/
#include "Forward.h"


namespace image {

    namespace pixel {
        
        using Byte = uint8_t;
        using Byte2 = uint16_t;
        using Byte4 = uint32_t;

        static const Byte BLACK8 { 0 };
        static const Byte WHITE8 { 255 };

        template <int N> int bitVal() { return 1 << N; }
        template <int Tn, int An> int bitProduct() { return bitVal<Tn>() * bitVal<An>(); }
        template <int Tn, int An, typename T = Byte, typename A = T> T mix(const T x, const T y, const A a) { return T(((bitVal<An>() - a) * x + a * y) / bitProduct<Tn, An>()); }
        
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


        struct RGB16_565 {
            Byte2 b : 5;
            Byte2 g : 6;
            Byte2 r : 5;
            
            RGB16_565() : b(BLACK8), g(BLACK8), r(BLACK8) {}
            RGB16_565(Byte pR, Byte pG, Byte pB) : b(pB), g(pG), r(pR) {}
        };
        
        
        Byte mix5_4(const Byte x, const Byte y, const Byte a) { return mix<5, 4>(x, y, a); }
        Byte mix6_4(const Byte x, const Byte y, const Byte a) { return mix<6, 4>(x, y, a); }
        
        
        template <typename P, typename S> const P mix(const P p0, const P p1, const S alpha) { return p0; }
        
        template <> const RGB16_565 mix(const RGB16_565 p0, const RGB16_565 p1, const Byte alpha);
        

    };

    template <typename F, typename S> class Pixel {
    public:
        using Format = F;
        using Storage = S;
        
        union {
            Format val;
            Storage raw;
        };
        
        Pixel() {};
        Pixel(Format v) : val(v) {}
        Pixel(Storage s) : raw(s) {}
    };

    using PixRGB565 = Pixel<pixel::RGB16_565, pixel::Byte2>;
    using PixRGB32 = Pixel<pixel::RGB32, pixel::Byte4>;
    using PixRGBA32 = Pixel<pixel::RGBA32, pixel::Byte4>;
    
    template <typename P> class PixelBlock {
    public:
        static const uint32_t WIDTH { 4 };
        static const uint32_t HEIGHT { WIDTH };
        static const uint32_t SIZE { WIDTH * HEIGHT };
        uint32_t getSize() const { return SIZE * sizeof(P); }
        
        P pixels[SIZE];
        
        PixelBlock() {}
        
        
        PixelBlock(const P* srcPixels) {
            setPixels(srcPixels);
        }
        
        void setPixels(const P* srcPixels) {
            memcpy(pixels, srcPixels, getSize());
        }
    };
    
    class BC {
    public:
        static int cpp;
        
        struct BC1 {
            PixRGB565 color0;
            PixRGB565 color1;
            pixel::Byte4 table;
        };
        struct BC4 {
            PixRGB565 color0;
            PixRGB565 color1;
            pixel::Byte4 table;
        };
    };
    
    template <typename T> class CompressedBlock {
    public:
        static const uint32_t SIZE { sizeof(T) };
        union {
            pixel::Byte bytes[SIZE];
            T bc;
        };
        
        CompressedBlock() {}
    };
    
    template <typename PB, typename CB> void compress(const PB& srcBlock, CB& dstBlock) { }
    template <typename PB, typename CB> void uncompress(const CB& srcBlock, PB& dstBlock) { }


    using PB_RGB32 = PixelBlock<PixRGB32>;
    using PB_RGBA32 = PixelBlock<PixRGBA32>;
    
    using CB_BC1 = CompressedBlock<BC::BC1>;
    using CB_BC4 = CompressedBlock<BC::BC4>;
    
    template <> void compress(const PB_RGB32& src, CB_BC1& dst);
    template <> void compress(const PB_RGBA32& src, CB_BC4& dst);

    template <> void uncompress(const CB_BC1& src, PB_RGB32& dst);
    template <> void uncompress(const CB_BC4& src, PB_RGBA32& dst);
}