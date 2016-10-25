#include "Image.h"


int image::Pixel::cpp { 0 };

namespace image {

    template <> void compress(const PB_RGB32& src, CB_8& dst) {
    
    for (auto& b : dst.bytes) {
        b = 12;
    }
}

template <> void uncompress(const CB_8& src, PB_RGB32& dst) {
    for (auto& b : dst.bytes) {
        b = 12;
    }
}

template <> void compress(const PB_RGBA32& src, CB_8& dst) {
    
}

template <> void uncompress(const CB_8& src, PB_RGBA32& dst) {

}

}