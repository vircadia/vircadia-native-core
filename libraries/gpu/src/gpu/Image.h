/*
 Let s try compressing images
 
*/
#include "Forward.h"


namespace image {

    class Pixel {
    public:
        using Byte = uint8_t;
        using Byte2 = uint16_t;
        using Byte4 = uint32_t;

        static const Byte BLACK8 { 0 };
        static const Byte WHITE8 { 255 };


        struct RGB32 {
            Byte r { BLACK };
            Byte g { BLACK };
            Byte b { BLACK };
            Byte x { WHITE };
        };
        
        struct RGBA32 {
            Byte r { BLACK };
            Byte g { BLACK };
            Byte b { BLACK };
            Byte a { WHITE };
        };


        struct RGB16_565 {
            Byte2 b : 5;
            Byte2 g : 6;
            Byte2 r : 5;
        };

        static int cpp;
    };

    template <typename P> class PixelBlock {
    public:
        static const uint32_t WIDTH { 4 };
        static const uint32_t HEIGHT { WIDTH };
        static const uint32_t SIZE { WIDTH * HEIGHT };
        uint32_t getByteSize() const { return SIZE * sizeof(P); }
        
        P pixels[SIZE];
        
        PixelBlock() {}
        
        
        PixelBlock(const P* srcPixels) {
            setPixels(srcPixels);
        }
        
        void setPixels(const P* srcPixels) {
            memcpy(pixels, srcPixels, getByteSize());
        }
    };
    
    template <int Size> class CompressedBlock {
    public:
        uint8_t bytes[Size];
    };
    
    class BC {
    public:
        struct BC1 {
            Pixel::RGB16_565 color_0;
            Pixel::RGB16_565 color_1;
            Pixel::Byte4 table;
        };
    };


    template <typename PB, typename CB> void compress(const PB& srcBlock, CB& dstBlock) { }
    template <typename PB, typename CB> void uncompress(const CB& srcBlock, PB& dstBlock) { }


    using PB_RGB32 = PixelBlock<Pixel::RGB32>;
    using PB_RGBA32 = PixelBlock<Pixel::RGBA32>;
    
    using CB_8 = CompressedBlock<8>;
    using CB_16 = CompressedBlock<16>;
    
    template <> void compress(const PB_RGB32& src, CB_8& dst);
    template <> void compress(const PB_RGBA32& src, CB_8& dst);

    template <> void uncompress(const CB_8& src, PB_RGB32& dst);
    template <> void uncompress(const CB_8& src, PB_RGBA32& dst);
}