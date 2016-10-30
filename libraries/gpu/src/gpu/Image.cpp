#include "Image.h"


int image::BC::cpp { 0 };

namespace image {

    namespace pixel {
        template <> const RGB16_565 mix(const RGB16_565 p0, const RGB16_565 p1, const Byte alpha) {
            return RGB16_565(
                mix5_4(p0.r, p1.r, alpha),
                mix6_4(p0.g, p1.g, alpha),
                mix5_4(p0.b, p1.b, alpha));
        }
    }
    
template <> void compress(const PB_RGB32& src, CB_BC1& dst) {
    
    for (auto& b : dst.bytes) {
        b = 12;
    }
}

template <> void uncompress(const CB_BC1& src, PB_RGB32& dst) {
    auto bc1 = src.bc;
    
    auto c0 = bc1.color0.val;
    auto c1 = bc1.color1.val;
    
    for (int i = 0; i < PB_RGB32::SIZE; ++i) {
        //dst.pixels[i] = ;
        auto r = pixel::mix(
                       c0,
                       c1,
                       (pixel::Byte)bc1.table);
    }
}

template <> void compress(const PB_RGBA32& src, CB_BC4& dst) {
    
}

template <> void uncompress(const CB_BC4& src, PB_RGBA32& dst) {

}

}