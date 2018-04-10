//
//  Created by Bradley Austin Davis on 2017/01/25
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TextureTable.h"
#include "Texture.h"

#include <shared/GlobalAppProperties.h>
using namespace gpu;


const size_t TextureTable::COUNT{ TEXTURE_TABLE_COUNT };

TextureTable::TextureTable() { }

TextureTable::TextureTable(const std::initializer_list<TexturePointer>& textures) {
    auto max = std::min<size_t>(COUNT, textures.size());
    auto itr = textures.begin();
    size_t index = 0;
    while (itr != textures.end() && index < max) {
        setTexture(index, *itr);
        ++index;
    }
}

TextureTable::TextureTable(const Array& textures) : _textures(textures) {
}

void TextureTable::setTexture(size_t index, const TexturePointer& texturePointer) {
    if (index >= COUNT || _textures[index] == texturePointer) {
        return;
    }
    {
        Lock lock(_mutex);
        ++_stamp;
        _textures[index] = texturePointer;
    }
}

void TextureTable::setTexture(size_t index, const TextureView& textureView) {
    setTexture(index, textureView._texture);
}

TextureTable::Array TextureTable::getTextures() const {
     Array result; 
     {
         Lock lock(_mutex);
         result = _textures;
     }
     return result; 
}
