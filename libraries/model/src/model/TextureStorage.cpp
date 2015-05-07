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
TextureStorage::TextureStorage(const QUrl& url, Texture::Type type ) : Texture::Storage(),
     _url(url),
     _type(type) {
    init();
}

TextureStorage::~TextureStorage() {
}

void TextureStorage::init() {
    _gpuTexture = TexturePointer(Texture::createFromStorage(this));
}

