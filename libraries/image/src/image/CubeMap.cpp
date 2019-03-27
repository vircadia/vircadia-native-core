//
//  CubeMap.h
//  image/src/image
//
//  Created by Olivier Prat on 03/27/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "CubeMap.h"

using namespace image;

CubeMap::CubeMap(int width, int height, int mipCount) :
    _width(width), _height(height) {
    assert(mipCount >0 && _width > 0 && _height > 0);
    _mips.resize(mipCount);
    for (auto mipLevel = 0; mipLevel < mipCount; mipLevel++) {
        auto mipWidth = std::max(1, width >> mipLevel);
        auto mipHeight = std::max(1, height >> mipLevel);
        auto mipPixelCount = mipWidth * mipHeight;

        for (auto& face : _mips[mipLevel]) {
            face.resize(mipPixelCount);
        }
    }
}
