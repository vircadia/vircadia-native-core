#include "TextureUtils.h"


int image::Pixel::cpp { 0 };

namespace image {
template <> void compress(const PB_RGB32& src, CB_8& dst) {
    
    for (auto& b : dst.bytes) {
        b = 12;
    }
}

template <> void compress(const PB_RGBA32& src, CB_8& dst) {
    
}
}