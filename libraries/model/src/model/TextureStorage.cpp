//
//  TextureStorage.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 5/6/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TextureStorage.h"

using namespace model;
using namespace gpu;

// TextureStorage
TextureStorage::TextureStorage()
{/* : Texture::Storage()//, 
  //  _gpuTexture(Texture::createFromStorage(this))*/
}

TextureStorage::~TextureStorage() {
}

void TextureStorage::reset(const QUrl& url, const TextureUsage& usage) {
    _imageUrl = url;
    _usage = usage;
}

void TextureStorage::resetTexture(gpu::Texture* texture) {
    _gpuTexture.reset(texture);
}

bool TextureStorage::isDefined() const {
    if (_gpuTexture) {
        return _gpuTexture->isDefined();
    } else {
        return false;
    }
}


void TextureMap::setTextureStorage(TextureStoragePointer& texStorage) {
    _textureStorage = texStorage;
}

bool TextureMap::isDefined() const {
    if (_textureStorage) {
        return _textureStorage->isDefined();
    } else {
        return false;
    }
}

gpu::TextureView TextureMap::getTextureView() const {
    if (_textureStorage) {
        return gpu::TextureView(_textureStorage->getGPUTexture(), 0);
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

