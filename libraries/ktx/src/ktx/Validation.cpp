//
//  Created by Bradley Austin Davis on 2017/05/13
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "KTX.h"

#include <unordered_set>
#include <QDebug>

using namespace ktx;

static const std::unordered_set<uint32_t> VALID_GL_TYPES {
    (uint32_t)GLType::UNSIGNED_BYTE,
    (uint32_t)GLType::BYTE,
    (uint32_t)GLType::UNSIGNED_SHORT,
    (uint32_t)GLType::SHORT,
    (uint32_t)GLType::UNSIGNED_INT,
    (uint32_t)GLType::INT,
    (uint32_t)GLType::HALF_FLOAT,
    (uint32_t)GLType::FLOAT,
    (uint32_t)GLType::UNSIGNED_BYTE_3_3_2,
    (uint32_t)GLType::UNSIGNED_BYTE_2_3_3_REV,
    (uint32_t)GLType::UNSIGNED_SHORT_5_6_5,
    (uint32_t)GLType::UNSIGNED_SHORT_5_6_5_REV,
    (uint32_t)GLType::UNSIGNED_SHORT_4_4_4_4,
    (uint32_t)GLType::UNSIGNED_SHORT_4_4_4_4_REV,
    (uint32_t)GLType::UNSIGNED_SHORT_5_5_5_1,
    (uint32_t)GLType::UNSIGNED_SHORT_1_5_5_5_REV,
    (uint32_t)GLType::UNSIGNED_INT_8_8_8_8,
    (uint32_t)GLType::UNSIGNED_INT_8_8_8_8_REV,
    (uint32_t)GLType::UNSIGNED_INT_10_10_10_2,
    (uint32_t)GLType::UNSIGNED_INT_2_10_10_10_REV,
    (uint32_t)GLType::UNSIGNED_INT_24_8,
    (uint32_t)GLType::UNSIGNED_INT_10F_11F_11F_REV,
    (uint32_t)GLType::UNSIGNED_INT_5_9_9_9_REV,
    (uint32_t)GLType::FLOAT_32_UNSIGNED_INT_24_8_REV,
};

static const std::unordered_set<uint32_t> VALID_GL_FORMATS {
    (uint32_t)GLFormat::STENCIL_INDEX,
    (uint32_t)GLFormat::DEPTH_COMPONENT,
    (uint32_t)GLFormat::DEPTH_STENCIL,
    (uint32_t)GLFormat::RED,
    (uint32_t)GLFormat::GREEN,
    (uint32_t)GLFormat::BLUE,
    (uint32_t)GLFormat::RG,
    (uint32_t)GLFormat::RGB,
    (uint32_t)GLFormat::RGBA,
    (uint32_t)GLFormat::BGR,
    (uint32_t)GLFormat::BGRA,
    (uint32_t)GLFormat::RG_INTEGER,
    (uint32_t)GLFormat::RED_INTEGER,
    (uint32_t)GLFormat::GREEN_INTEGER,
    (uint32_t)GLFormat::BLUE_INTEGER,
    (uint32_t)GLFormat::RGB_INTEGER,
    (uint32_t)GLFormat::RGBA_INTEGER,
    (uint32_t)GLFormat::BGR_INTEGER,
    (uint32_t)GLFormat::BGRA_INTEGER,
};

static const std::unordered_set<uint32_t> VALID_GL_INTERNAL_FORMATS {
    (uint32_t)GLInternalFormat::R8,
    (uint32_t)GLInternalFormat::R8_SNORM,
    (uint32_t)GLInternalFormat::R16,
    (uint32_t)GLInternalFormat::R16_SNORM,
    (uint32_t)GLInternalFormat::RG8,
    (uint32_t)GLInternalFormat::RG8_SNORM,
    (uint32_t)GLInternalFormat::RG16,
    (uint32_t)GLInternalFormat::RG16_SNORM,
    (uint32_t)GLInternalFormat::R3_G3_B2,
    (uint32_t)GLInternalFormat::RGB4,
    (uint32_t)GLInternalFormat::RGB5,
    (uint32_t)GLInternalFormat::RGB565,
    (uint32_t)GLInternalFormat::RGB8,
    (uint32_t)GLInternalFormat::RGB8_SNORM,
    (uint32_t)GLInternalFormat::RGB10,
    (uint32_t)GLInternalFormat::RGB12,
    (uint32_t)GLInternalFormat::RGB16,
    (uint32_t)GLInternalFormat::RGB16_SNORM,
    (uint32_t)GLInternalFormat::RGBA2,
    (uint32_t)GLInternalFormat::RGBA4,
    (uint32_t)GLInternalFormat::RGB5_A1,
    (uint32_t)GLInternalFormat::RGBA8,
    (uint32_t)GLInternalFormat::RGBA8_SNORM,
    (uint32_t)GLInternalFormat::RGB10_A2,
    (uint32_t)GLInternalFormat::RGB10_A2UI,
    (uint32_t)GLInternalFormat::RGBA12,
    (uint32_t)GLInternalFormat::RGBA16,
    (uint32_t)GLInternalFormat::RGBA16_SNORM,
    (uint32_t)GLInternalFormat::SRGB8,
    (uint32_t)GLInternalFormat::SRGB8_ALPHA8,
    (uint32_t)GLInternalFormat::R16F,
    (uint32_t)GLInternalFormat::RG16F,
    (uint32_t)GLInternalFormat::RGB16F,
    (uint32_t)GLInternalFormat::RGBA16F,
    (uint32_t)GLInternalFormat::R32F,
    (uint32_t)GLInternalFormat::RG32F,
    (uint32_t)GLInternalFormat::RGBA32F,
    (uint32_t)GLInternalFormat::R11F_G11F_B10F,
    (uint32_t)GLInternalFormat::RGB9_E5,
    (uint32_t)GLInternalFormat::R8I,
    (uint32_t)GLInternalFormat::R8UI,
    (uint32_t)GLInternalFormat::R16I,
    (uint32_t)GLInternalFormat::R16UI,
    (uint32_t)GLInternalFormat::R32I,
    (uint32_t)GLInternalFormat::R32UI,
    (uint32_t)GLInternalFormat::RG8I,
    (uint32_t)GLInternalFormat::RG8UI,
    (uint32_t)GLInternalFormat::RG16I,
    (uint32_t)GLInternalFormat::RG16UI,
    (uint32_t)GLInternalFormat::RG32I,
    (uint32_t)GLInternalFormat::RG32UI,
    (uint32_t)GLInternalFormat::RGB8I,
    (uint32_t)GLInternalFormat::RGB8UI,
    (uint32_t)GLInternalFormat::RGB16I,
    (uint32_t)GLInternalFormat::RGB16UI,
    (uint32_t)GLInternalFormat::RGB32I,
    (uint32_t)GLInternalFormat::RGB32UI,
    (uint32_t)GLInternalFormat::RGBA8I,
    (uint32_t)GLInternalFormat::RGBA8UI,
    (uint32_t)GLInternalFormat::RGBA16I,
    (uint32_t)GLInternalFormat::RGBA16UI,
    (uint32_t)GLInternalFormat::RGBA32I,
    (uint32_t)GLInternalFormat::RGBA32UI,
    (uint32_t)GLInternalFormat::DEPTH_COMPONENT16,
    (uint32_t)GLInternalFormat::DEPTH_COMPONENT24,
    (uint32_t)GLInternalFormat::DEPTH_COMPONENT32,
    (uint32_t)GLInternalFormat::DEPTH_COMPONENT32F,
    (uint32_t)GLInternalFormat::DEPTH24_STENCIL8,
    (uint32_t)GLInternalFormat::DEPTH32F_STENCIL8,
    (uint32_t)GLInternalFormat::STENCIL_INDEX1,
    (uint32_t)GLInternalFormat::STENCIL_INDEX4,
    (uint32_t)GLInternalFormat::STENCIL_INDEX8,
    (uint32_t)GLInternalFormat::STENCIL_INDEX16,
};

static const std::unordered_set<uint32_t> VALID_GL_INTERNAL_COMPRESSED_FORMATS {
    (uint32_t)GLInternalFormat::COMPRESSED_RED,
    (uint32_t)GLInternalFormat::COMPRESSED_RG,
    (uint32_t)GLInternalFormat::COMPRESSED_RGB,
    (uint32_t)GLInternalFormat::COMPRESSED_RGBA,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB_ALPHA,
    (uint32_t)GLInternalFormat::COMPRESSED_ETC1_RGB8_OES,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,
    (uint32_t)GLInternalFormat::COMPRESSED_RED_RGTC1,
    (uint32_t)GLInternalFormat::COMPRESSED_SIGNED_RED_RGTC1,
    (uint32_t)GLInternalFormat::COMPRESSED_RG_RGTC2,
    (uint32_t)GLInternalFormat::COMPRESSED_SIGNED_RG_RGTC2,
    (uint32_t)GLInternalFormat::COMPRESSED_RGBA_BPTC_UNORM,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
    (uint32_t)GLInternalFormat::COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
    (uint32_t)GLInternalFormat::COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
    (uint32_t)GLInternalFormat::COMPRESSED_RGB8_ETC2,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB8_ETC2,
    (uint32_t)GLInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    (uint32_t)GLInternalFormat::COMPRESSED_RGBA8_ETC2_EAC,
    (uint32_t)GLInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
    (uint32_t)GLInternalFormat::COMPRESSED_R11_EAC,
    (uint32_t)GLInternalFormat::COMPRESSED_SIGNED_R11_EAC,
    (uint32_t)GLInternalFormat::COMPRESSED_RG11_EAC,
    (uint32_t)GLInternalFormat::COMPRESSED_SIGNED_RG11_EAC,
};

static const std::unordered_set<uint32_t> VALID_GL_BASE_INTERNAL_FORMATS {
    (uint32_t)GLBaseInternalFormat::DEPTH_COMPONENT,
    (uint32_t)GLBaseInternalFormat::DEPTH_STENCIL,
    (uint32_t)GLBaseInternalFormat::RED,
    (uint32_t)GLBaseInternalFormat::RG,
    (uint32_t)GLBaseInternalFormat::RGB,
    (uint32_t)GLBaseInternalFormat::RGBA,
    (uint32_t)GLBaseInternalFormat::STENCIL_INDEX,
};

bool Header::isValid() const {
    if (0 != memcmp(identifier, IDENTIFIER.data(), IDENTIFIER_LENGTH)) {
        qDebug() << "Invalid header identifier";
        return false;
    }

    if (endianness != ENDIAN_TEST && endianness != REVERSE_ENDIAN_TEST) {
        qDebug("Invalid endian marker 0x%x", endianness);
        return false;
    }

    //
    // GL enum validity
    //
    if (VALID_GL_BASE_INTERNAL_FORMATS.count(glBaseInternalFormat) != 1) {
        qDebug("Invalid base internal format 0x%x", glBaseInternalFormat);
        return false;
    }

    if (isCompressed()) {
        if (glType != COMPRESSED_TYPE) {
            qDebug("Invalid type for compressed texture 0x%x", glType);
            return false;
        }

        if (glTypeSize != COMPRESSED_TYPE_SIZE) {
            qDebug("Invalid type size for compressed texture %d", glTypeSize);
            return false;
        }

        if (VALID_GL_INTERNAL_COMPRESSED_FORMATS.count(glInternalFormat) != 1) {
            qDebug("Invalid compressed internal format 0x%x", glInternalFormat);
            return false;
        }
    } else {
        if (VALID_GL_TYPES.count(glType) != 1) {
            qDebug("Invalid type 0x%x", glType);
            return false;
        }

        if (VALID_GL_FORMATS.count(glFormat) != 1) {
            qDebug("Invalid format 0x%x", glFormat);
            return false;
        }

        if (VALID_GL_INTERNAL_FORMATS.count(glInternalFormat) != 1) {
            qDebug("Invalid internal format 0x%x", glInternalFormat);
            return false;
        }
    }

    //
    // Dimensions validity
    //

    // Textures must at least have a width
    // If they have a depth, they must have a height
    if ((pixelWidth == 0) || (pixelDepth != 0 && pixelHeight == 0)) {
        qDebug() << "Invalid dimensions " << pixelWidth << "x" << pixelHeight << "x" << pixelDepth;
        return false;
    }


    if (numberOfFaces != 1 && numberOfFaces != NUM_CUBEMAPFACES) {
        qDebug() << "Invalid number of faces " << numberOfFaces;
        return false;
    }

    // FIXME validate numberOfMipmapLevels based on the dimensions?

    if ((bytesOfKeyValueData % 4) != 0) {
        qDebug() << "Invalid keyvalue data size " << bytesOfKeyValueData;
        return false;
    }

    return true;
}

struct AlignedStreamBuffer {
    AlignedStreamBuffer(size_t size, const uint8_t* data) 
        : _size(size), _data(data) { }

    AlignedStreamBuffer(const StoragePointer& storage) 
        : AlignedStreamBuffer(storage->size(), storage->data()) { }


    template<typename T>
    bool read(T& t) {
        // Ensure we don't read more than we have
        if (sizeof(T) > _size) {
            return false;
        }

        // Grab the data
        memcpy(&t, _data, sizeof(T));

        // Advance the pointer
        return skip(sizeof(T));
    }

    bool skip(size_t skipSize) {
        skipSize = ktx::evalPaddedSize(skipSize);
        if (skipSize > _size) {
            return false;
        }
        _data += skipSize;
        _size -= skipSize;
        return true;
    }

    AlignedStreamBuffer front(size_t size) const {
        return AlignedStreamBuffer { std::min(size, _size), _data };
    }

    bool empty() const {
        return _size == 0;
    }

private:
    size_t _size;
    const uint8_t* _data;
};

bool validateKeyValueData(AlignedStreamBuffer kvbuffer) {
    while (!kvbuffer.empty()) {
        uint32_t keyValueSize;
        // Try to fetch the size of the next key value block
        if (!kvbuffer.read(keyValueSize)) {
            qDebug() << "Unable to read past key value size";
            return false;
        }
        if (!kvbuffer.skip(keyValueSize)) {
            qDebug() << "Unable to skip past key value data";
            return false;
        }
    }
    
    return true;
}

bool KTX::validate(const StoragePointer& src) {
    if (!checkAlignment(src->size())) {
        // All KTX data is 4-byte aligned
        qDebug() << "Invalid size, not 4 byte aligned";
        return false;
    }

    Header header;
    AlignedStreamBuffer buffer { src };
    if (!buffer.read(header)) {
        qDebug() << "Unable to read header";
        return false;
    }

    // Basic header validation, are the enums and size valid?
    if (!header.isValid()) {
        qDebug() << "Invalid header";
        return false;
    }

    // Validate the key value pairs
    if (!validateKeyValueData(buffer.front(header.bytesOfKeyValueData))) {
        qDebug() << "Invalid key value data";
        return false;
    }

    // now skip the KV data
    if (!buffer.skip(header.bytesOfKeyValueData)) {
        qDebug() << "Unable to read past key value data";
        return false;
    }


    // Validate the images
    for (uint32_t mip = 0; mip < header.numberOfMipmapLevels; ++mip) {
        uint32_t imageSize;
        if (!buffer.read(imageSize)) {
            qDebug() << "Unable to read image size";
            return false;
        }

        uint32_t arrayElements = header.numberOfArrayElements == 0 ? 1 : header.numberOfArrayElements;
        for (uint32_t arrayElement = 0; arrayElement < arrayElements; ++arrayElement) {
            for (uint8_t face = 0; face < header.numberOfFaces; ++face) {
                if (!buffer.skip(imageSize)) {
                    qDebug() << "Unable to skip past image data";
                    return false;
                }
            }
        }
    }

    // The buffer should be empty afer we've skipped all of the KTX data
    if (!buffer.empty()) {
        return false;
    }

    return true;
}



bool KTX::isValid() const {
    if (!_header.isValid()) {
        return false;
    }

    if (_images.size() != _header.numberOfMipmapLevels) {
        return false;
    }

    const auto start = _storage->data();
    const auto end = start + _storage->size();

    // FIXME, do key value checks?

    for (const auto& image : _images) {
        if (image._numFaces != _header.numberOfFaces) {
            return false;
        }

        for (const auto& facePointer : image._faceBytes) {
            if (facePointer + image._faceSize > end) {
                return false;
            }
        }
    }


    for (uint8_t mip = 0; mip < _header.numberOfMipmapLevels; ++mip) {
        for (uint8_t face = 0; face < _header.numberOfFaces; ++face) {
            auto faceStorage = getMipFaceTexelsData(mip, face);
            // The face start offset must be 4 byte aligned
            if (!checkAlignment(faceStorage->data() - start)) {
                return false;
            }

            // The face size must be 4 byte aligned
            if (!checkAlignment(faceStorage->size())) {
                return false;
            }
        }
    }

    return true;
}