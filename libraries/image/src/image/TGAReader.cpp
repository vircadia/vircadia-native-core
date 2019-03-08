//
//  TGAReader.cpp
//  image/src/image
//
//  Created by Ryan Huffman
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TGAReader.h"

#include <QIODevice>
#include <QDebug>

QImage image::readTGA(QIODevice& content) {
    enum class TGAImageType : uint8_t {
        NoImageData = 0,
        UncompressedColorMapped = 1,
        UncompressedTrueColor = 2,
        UncompressedBlackWhite = 3,
        RunLengthEncodedColorMapped = 9,
        RunLengthEncodedTrueColor = 10,
        RunLengthEncodedBlackWhite = 11,
    };

    struct TGAHeader {
        uint8_t idLength;
        uint8_t colorMapType;
        TGAImageType imageType;
        struct {
            uint64_t firstEntryIndex : 16;
            uint64_t length : 16;
            uint64_t entrySize : 8;
        } colorMap;
        uint16_t xOrigin;
        uint16_t yOrigin;
        uint16_t width;
        uint16_t height;
        uint8_t pixelDepth;
        struct {
            uint8_t attributeBitsPerPixel : 4;
            uint8_t orientation : 2;
            uint8_t padding : 2;
        } imageDescriptor;
    };

    constexpr bool WANT_DEBUG { false };
    constexpr size_t TGA_HEADER_SIZE_BYTES { 18 };

    TGAHeader header;

    if (content.isSequential()) {
        qWarning() << "TGA - Sequential devices are not supported for reading";
        return QImage();
    }

    if (content.bytesAvailable() < TGA_HEADER_SIZE_BYTES) {
        qWarning() << "TGA - Unexpectedly reached end of file";
        return QImage();
    }

    content.read((char*)&header.idLength, 1);
    content.read((char*)&header.colorMapType, 1);
    content.read((char*)&header.imageType, 1);
    content.read((char*)&header.colorMap, 5);
    content.read((char*)&header.xOrigin, 2);
    content.read((char*)&header.yOrigin, 2);
    content.read((char*)&header.width, 2);
    content.read((char*)&header.height, 2);
    content.read((char*)&header.pixelDepth, 1);
    content.read((char*)&header.imageDescriptor, 1);

    if (WANT_DEBUG) {
        qDebug() << "Id Length: " << (int)header.idLength;
        qDebug() << "Color map: " << (int)header.colorMap.firstEntryIndex << header.colorMap.length << header.colorMap.entrySize;
        qDebug() << "Color map type: " << (int)header.colorMapType;
        qDebug() << "Image type: " << (int)header.imageType;
        qDebug() << "Origin: " << header.xOrigin << header.yOrigin;
        qDebug() << "Size: " << header.width << header.height;
        qDebug() << "Depth: " << header.pixelDepth;
        qDebug() << "Image desc: " << header.imageDescriptor.attributeBitsPerPixel << header.imageDescriptor.orientation;
    }

    if (header.xOrigin != 0 || header.yOrigin != 0) {
        qWarning() << "TGA - origin not supporter";
        return QImage();
    }

    if (!(header.pixelDepth == 24 && header.imageDescriptor.attributeBitsPerPixel == 0) && header.pixelDepth != 32) {
        qWarning() << "TGA - Only pixel depths of 24 (with no alpha) and 32 bits are supported";
        return QImage();
    }

    if (header.imageDescriptor.attributeBitsPerPixel != 0 && header.imageDescriptor.attributeBitsPerPixel != 8) {
        qWarning() << "TGA - Only 0 or 8 bits for the alpha channel is supported";
        return QImage();
    }

    char alphaMask = header.imageDescriptor.attributeBitsPerPixel == 8 ? 0x00 : 0xFF;
    int bytesPerPixel = header.pixelDepth / 8;

    content.skip(header.idLength);
    if (header.imageType == TGAImageType::UncompressedTrueColor) {
        qint64 minimumSize = header.width * header.height * bytesPerPixel;
        if (content.bytesAvailable() < minimumSize) {
            qWarning() << "TGA - Unexpectedly reached end of file";
            return QImage();
        }

        QImage image{ header.width, header.height, QImage::Format_ARGB32 };
        for (int y = 0; y < header.height; ++y) {
            char* line = (char*)image.scanLine(y);
            for (int x = 0; x < header.width; ++x) {
                content.read(line, bytesPerPixel);
                *(line + 3) |= alphaMask;

                line += 4;
            }
        }
        return image;
    } else if (header.imageType == TGAImageType::RunLengthEncodedTrueColor) {
        QImage image{ header.width, header.height, QImage::Format_ARGB32 };

        for (int y = 0; y < header.height; ++y) {
            char* line = (char*)image.scanLine(y);
            int col = 0;
            while (col < header.width) {
                constexpr char IS_REPETITION_MASK{ (char)0x80 };
                constexpr char LENGTH_MASK{ (char)0x7f };
                char repetition;
                if (content.read(&repetition, 1) != 1) {
                    qWarning() << "TGA - Unexpectedly reached end of file";
                    return QImage();
                }
                bool isRepetition = repetition & IS_REPETITION_MASK;

                // The length in `repetition` is always 1 less than the number of following pixels,
                // so we need to increment it by 1. Because of this, the length is never 0.
                int length = (repetition & LENGTH_MASK) + 1;

                if (isRepetition) {
                    // Read into temporary buffer
                    char color[4];
                    if (content.read(color, bytesPerPixel) != bytesPerPixel) {
                        qWarning() << "TGA - Unexpectedly reached end of file";
                        return QImage();
                    }
                    color[3] |= alphaMask;

                    // Copy `length` number of times
                    col += length;
                    while (length-- > 0) {
                        *line = color[0];
                        *(line + 1) = color[1];
                        *(line + 2) = color[2];
                        *(line + 3) = color[3];

                        line += 4;
                    }
                } else {
                    qint64 minimumSize = length * bytesPerPixel;
                    if (content.bytesAvailable() < minimumSize) {
                        qWarning() << "TGA - Unexpectedly reached end of file";
                        return QImage();
                    }

                    // Read in `length` number of pixels
                    col += length;
                    while (length-- > 0) {
                        content.read(line, bytesPerPixel);
                        *(line + 3) |= alphaMask;

                        line += 4;
                    }
                }
            }
        }
        return image;
    } else {
        qWarning() << "TGA - Unsupported image type: " << (int)header.imageType;
    }

    return QImage();
}
