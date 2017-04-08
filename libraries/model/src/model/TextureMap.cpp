//
//  TextureMap.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 5/6/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TextureMap.h"

using namespace model;
using namespace gpu;

void TextureMap::setTextureSource(TextureSourcePointer& textureSource) {
    _textureSource = textureSource;
}

bool TextureMap::isDefined() const {
    if (_textureSource) {
        return _textureSource->isDefined();
    } else {
        return false;
    }
}

gpu::TextureView TextureMap::getTextureView() const {
    if (_textureSource) {
        return gpu::TextureView(_textureSource->getGPUTexture(), 0);
    } else {
        return gpu::TextureView();
    }
}

void TextureMap::setTextureTransform(const Transform& texcoordTransform) {
    _texcoordTransform = texcoordTransform;
}

void TextureMap::setLightmapOffsetScale(float offset, float scale) {
    _lightmapOffsetScale.x = offset;
    _lightmapOffsetScale.y = scale;
}
