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
    header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 4, ktx::GLFormat::BGRA, ktx::GLInternalFormat_Uncompressed::RGBA8, ktx::GLBaseInternalFormat::RGBA);
    header.pixelWidth = texture.getWidth();
    header.pixelHeight = texture.getHeight();
    header.numberOfMipmapLevels = texture.mipLevels();

    ktx::Images images;
    for (int level = 0; level < header.numberOfMipmapLevels; level++) {
        auto mip = texture.accessStoredMipFace(level);
        if (mip) {
            images.emplace_back(ktx::Image(mip->getSize(), 0, mip->readData()));
        }
    }

    auto ktxBuffer = ktx::KTX::create(header, images);
    return ktxBuffer;
}
TexturePointer Texture::unserialize(const ktx::KTXUniquePointer& srcData) {

    const auto& header = *srcData->getHeader();

    return nullptr;
}