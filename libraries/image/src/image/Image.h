#pragma once
//
//  Image.h
//  image/src/Image
//
//  Created by Olivier Prat on 29/3/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_image_Image_h
#define hifi_image_Image_h

#include <QImage>

#include "ColorChannel.h"

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <GLMHelpers.h>

namespace image {

    class Image {
    public:

        enum Format {
            Format_Invalid = QImage::Format_Invalid,
            Format_Mono = QImage::Format_Mono,
            Format_MonoLSB = QImage::Format_MonoLSB,
            Format_Indexed8 = QImage::Format_Indexed8,
            Format_RGB32 = QImage::Format_RGB32,
            Format_ARGB32 = QImage::Format_ARGB32,
            Format_ARGB32_Premultiplied = QImage::Format_ARGB32_Premultiplied,
            Format_RGB16 = QImage::Format_RGB16,
            Format_ARGB8565_Premultiplied = QImage::Format_ARGB8565_Premultiplied,
            Format_RGB666 = QImage::Format_RGB666,
            Format_ARGB6666_Premultiplied = QImage::Format_ARGB6666_Premultiplied,
            Format_RGB555 = QImage::Format_RGB555,
            Format_ARGB8555_Premultiplied = QImage::Format_ARGB8555_Premultiplied,
            Format_RGB888 = QImage::Format_RGB888,
            Format_RGB444 = QImage::Format_RGB444,
            Format_ARGB4444_Premultiplied = QImage::Format_ARGB4444_Premultiplied,
            Format_RGBX8888 = QImage::Format_RGBX8888,
            Format_RGBA8888 = QImage::Format_RGBA8888,
            Format_RGBA8888_Premultiplied = QImage::Format_RGBA8888_Premultiplied,
            Format_Grayscale8 = QImage::Format_Grayscale8,
            Format_R11G11B10F = QImage::Format_RGB30,
            Format_PACKED_FLOAT = Format_R11G11B10F
        };

        using AspectRatioMode = Qt::AspectRatioMode;
        using TransformationMode = Qt::TransformationMode;

        Image() {}
        Image(int width, int height, Format format) : _data(width, height, (QImage::Format)format) {}
        Image(const QImage& data) : _data(data) {}
        void operator=(const QImage& image) {
            _data = image;
        }

        bool isNull() const { return _data.isNull(); }

        Format getFormat() const { return (Format)_data.format(); }
        bool hasAlphaChannel() const { return _data.hasAlphaChannel(); }

        glm::uint32 getWidth() const { return (glm::uint32)_data.width(); }
        glm::uint32 getHeight() const { return (glm::uint32)_data.height(); }
        glm::uvec2 getSize() const { return toGlm(_data.size()); }
        size_t getByteCount() const { return _data.byteCount(); }
        size_t getBytesPerLineCount() const { return _data.bytesPerLine(); }

        QRgb getPixel(int x, int y) const { return _data.pixel(x, y); }
        void setPixel(int x, int y, QRgb value) {
            _data.setPixel(x, y, value);
        }

        glm::uint8* editScanLine(int y) { return _data.scanLine(y); }
        const glm::uint8* getScanLine(int y) const { return _data.scanLine(y); }
        const glm::uint8* getBits() const { return _data.constBits(); }

        Image getScaled(glm::uvec2 newSize, AspectRatioMode ratioMode, TransformationMode transformationMode = Qt::SmoothTransformation) const;
        Image getConvertedToFormat(Format newFormat) const;
        Image getSubImage(QRect rect) const;
        Image getMirrored(bool horizontal, bool vertical) const;

        // Inplace transformations
        void invertPixels();

    private:

        QImage _data;
    };

} // namespace image

#endif // hifi_image_Image_h
