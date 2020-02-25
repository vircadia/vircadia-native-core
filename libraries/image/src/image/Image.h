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
            Format_PACKED_FLOAT = Format_R11G11B10F,
            // RGBA 32 bit single precision float per component
            Format_RGBAF = 100
        };

        using AspectRatioMode = Qt::AspectRatioMode;
        using TransformationMode = Qt::TransformationMode;

        Image() : _dims(0,0) {}
        Image(int width, int height, Format format);
        Image(const QImage& data) : _packedData(data), _dims(data.width(), data.height()), _format((Format)data.format()) {}
        Image(const Image &other) = default;
        void operator=(const QImage& other) {
            _packedData = other;
            _floatData.clear();
            _dims.x = other.width();
            _dims.y = other.height();
            _format = (Format)other.format();
        }

        void operator=(const Image& other) {
            if (&other != this) {
                _packedData = other._packedData;
                _floatData = other._floatData;
                _dims = other._dims;
                _format = other._format;
            }
        }

        bool isNull() const { return _packedData.isNull() && _floatData.empty(); }

        Format getFormat() const { return _format; }
        bool hasAlphaChannel() const { return _packedData.hasAlphaChannel() || _format == Format_RGBAF; }
        bool hasFloatFormat() const { return _format == Format_R11G11B10F || _format == Format_RGBAF; }

        glm::uint32 getWidth() const { return (glm::uint32)_dims.x; }
        glm::uint32 getHeight() const { return (glm::uint32)_dims.y; }
        glm::uvec2 getSize() const { return glm::uvec2(_dims); }
        size_t getByteCount() const;
        size_t getBytesPerLineCount() const;

        QRgb getPackedPixel(int x, int y) const {
            assert(_format != Format_RGBAF);
            return _packedData.pixel(x, y);
        }
        void setPackedPixel(int x, int y, QRgb value) {
            assert(_format != Format_RGBAF);
            _packedData.setPixel(x, y, value);
        }

        glm::vec4 getFloatPixel(int x, int y) const {
            assert(_format == Format_RGBAF);
            return _floatData[x + y*_dims.x];
        }
        void setFloatPixel(int x, int y, const glm::vec4& value) {
            assert(_format == Format_RGBAF);
            _floatData[x + y * _dims.x] = value;
        }

        glm::uint8* editScanLine(int y);
        const glm::uint8* getScanLine(int y) const;
        glm::uint8* editBits();
        const glm::uint8* getBits() const;

        Image getScaled(glm::uvec2 newSize, AspectRatioMode ratioMode, TransformationMode transformationMode = Qt::SmoothTransformation) const;
        Image getConvertedToFormat(Format newFormat) const;
        Image getSubImage(QRect rect) const;
        Image getMirrored(bool horizontal, bool vertical) const;

        // Inplace transformations
        void invertPixels();

    private:

        using FloatPixels = std::vector<glm::vec4>;

        // For QImage supported formats
        QImage _packedData;
        FloatPixels _floatData;
        glm::ivec2 _dims;
        Format _format { Format_Invalid };
    };

} // namespace image

#endif // hifi_image_Image_h
