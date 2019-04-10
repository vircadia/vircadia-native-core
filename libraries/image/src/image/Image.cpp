#include "Image.h"
#include "ImageLogging.h"
#include "TextureProcessing.h"

#include <nvtt/nvtt.h>

using namespace image;

Image Image::getScaled(glm::uvec2 dstSize, AspectRatioMode ratioMode, TransformationMode transformMode) const {
    if ((Image::Format)_data.format() == Image::Format_PACKED_FLOAT) {
        // Start by converting to full float
        glm::vec4* floatPixels = new glm::vec4[getWidth()*getHeight()];
        auto unpackFunc = getHDRUnpackingFunction();
        auto floatDataIt = floatPixels;
        for (glm::uint32 lineNb = 0; lineNb < getHeight(); lineNb++) {
            const glm::uint32* srcPixelIt = reinterpret_cast<const glm::uint32*>(getScanLine((int)lineNb));
            const glm::uint32* srcPixelEnd = srcPixelIt + getWidth();

            while (srcPixelIt < srcPixelEnd) {
                *floatDataIt = glm::vec4(unpackFunc(*srcPixelIt), 1.0f);
                ++srcPixelIt;
                ++floatDataIt;
            }
        }

        // Perform filtered resize with NVTT
        static_assert(sizeof(glm::vec4) == 4 * sizeof(float), "Assuming glm::vec4 holds 4 floats");
        nvtt::Surface surface;
        surface.setImage(nvtt::InputFormat_RGBA_32F, getWidth(), getHeight(), 1, floatPixels);
        delete[] floatPixels;

        nvtt::ResizeFilter filter = nvtt::ResizeFilter_Kaiser;
        if (transformMode == Qt::TransformationMode::FastTransformation) {
            filter = nvtt::ResizeFilter_Box;
        }
        surface.resize(dstSize.x, dstSize.y, 1, filter);

        // And convert back to original format
        QImage resizedImage((int)dstSize.x, (int)dstSize.y, (QImage::Format)Image::Format_PACKED_FLOAT);

        auto packFunc = getHDRPackingFunction();
        auto srcRedIt = reinterpret_cast<const float*>(surface.channel(0));
        auto srcGreenIt = reinterpret_cast<const float*>(surface.channel(1));
        auto srcBlueIt = reinterpret_cast<const float*>(surface.channel(2));
        for (glm::uint32 lineNb = 0; lineNb < dstSize.y; lineNb++) {
            glm::uint32* dstPixelIt = reinterpret_cast<glm::uint32*>(resizedImage.scanLine((int)lineNb));
            glm::uint32* dstPixelEnd = dstPixelIt + dstSize.x;

            while (dstPixelIt < dstPixelEnd) {
                *dstPixelIt = packFunc(glm::vec3(*srcRedIt, *srcGreenIt, *srcBlueIt));
                ++srcRedIt;
                ++srcGreenIt;
                ++srcBlueIt;
                ++dstPixelIt;
            }
        }
        return resizedImage;
    } else {
        return _data.scaled(fromGlm(dstSize), ratioMode, transformMode);
    }
}

Image Image::getConvertedToFormat(Format newFormat) const {
    assert(getFormat() != Format_PACKED_FLOAT);
    return _data.convertToFormat((QImage::Format)newFormat);
}

void Image::invertPixels() {
    _data.invertPixels(QImage::InvertRgba);
}

Image Image::getSubImage(QRect rect) const {
    return _data.copy(rect);
}

Image Image::getMirrored(bool horizontal, bool vertical) const {
    return _data.mirrored(horizontal, vertical);
}
