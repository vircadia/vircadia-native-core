//
//  Texture_ktx.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 2/16/2017.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "Texture.h"

#include <ktx/KTX.h>
using namespace gpu;

ktx::KTXUniquePointer Texture::serialize(const Texture& texture) {
    ktx::Header header;

    // From texture format to ktx format description
    auto texelFormat = texture.getTexelFormat();
    if ( !(   (texelFormat == Format::COLOR_RGBA_32)
           || (texelFormat == Format::COLOR_SRGBA_32)
          )) 
         return nullptr;

    header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 4, ktx::GLFormat::BGRA, ktx::GLInternalFormat_Uncompressed::RGBA8, ktx::GLBaseInternalFormat::RGBA);

    // Set Dimensions
    uint32_t numFaces = 1;
    switch (texture.getType()) {
    case TEX_1D: {
            if (texture.isArray()) {
                header.set1DArray(texture.getWidth(), texture.getNumSlices());
            } else {
                header.set1D(texture.getWidth());
            }
            break;
        }
    case TEX_2D: {
            if (texture.isArray()) {
                header.set2DArray(texture.getWidth(), texture.getHeight(), texture.getNumSlices());
            } else {
                header.set2D(texture.getWidth(), texture.getHeight());
            }
            break;
        }
    case TEX_3D: {
            if (texture.isArray()) {
                header.set3DArray(texture.getWidth(), texture.getHeight(), texture.getDepth(), texture.getNumSlices());
            } else {
                header.set3D(texture.getWidth(), texture.getHeight(), texture.getDepth());
            }
            break;
        }
    case TEX_CUBE: {
            if (texture.isArray()) {
                header.setCubeArray(texture.getWidth(), texture.getHeight(), texture.getNumSlices());
            } else {
                header.setCube(texture.getWidth(), texture.getHeight());
            }
            numFaces = 6;
            break;
        }
    default:
        return nullptr;
    }

    // Number level of mips coming
    header.numberOfMipmapLevels = texture.maxMip();

    ktx::Images images;
    for (uint32_t level = 0; level < header.numberOfMipmapLevels; level++) {
        auto mip = texture.accessStoredMipFace(level);
        if (mip) {
            if (numFaces == 1) {
                images.emplace_back(ktx::Image((uint32_t)mip->getSize(), 0, mip->readData()));
            } else {
                ktx::Image::FaceBytes cubeFaces(6);
                cubeFaces[0] = mip->readData();
                for (int face = 1; face < 6; face++) {
                    cubeFaces[face] = texture.accessStoredMipFace(level, face)->readData();
                }
                images.emplace_back(ktx::Image((uint32_t)mip->getSize(), 0, cubeFaces));
            }
        }
    }

    auto ktxBuffer = ktx::KTX::create(header, images);
    return ktxBuffer;
}

Texture* Texture::unserialize(Usage usage, TextureUsageType usageType, const ktx::KTXUniquePointer& srcData, const Sampler& sampler) {
    if (!srcData) {
        return nullptr;
    }
    const auto& header = *srcData->getHeader();

    Format mipFormat = Format::COLOR_SBGRA_32;
    Format texelFormat = Format::COLOR_SRGBA_32;

    // Find Texture Type based on dimensions
    Type type = TEX_1D;
    if (header.pixelWidth == 0) {
        return nullptr;
    } else if (header.pixelHeight == 0) {
        type = TEX_1D;
    } else if (header.pixelDepth == 0) {
        if (header.numberOfFaces == ktx::NUM_CUBEMAPFACES) {
            type = TEX_CUBE;
        } else {
            type = TEX_2D;
        }
    } else {
        type = TEX_3D;
    }

    auto tex = Texture::create( usageType,
                                type,
                                texelFormat,
                                header.getPixelWidth(),
                                header.getPixelHeight(),
                                header.getPixelDepth(),
                                1, // num Samples
                                header.getNumberOfSlices(),
                                sampler);

    tex->setUsage(usage);

    // Assing the mips availables
    uint16_t level = 0;
    for (auto& image : srcData->_images) {
        for (uint32_t face = 0; face < image._numFaces; face++) {
            tex->assignStoredMipFace(level, mipFormat, image._faceSize, image._faceBytes[face], face);
        }
        level++;
    }

    return tex;
}