//
//  Created by Bradley Austin Davis on 2016-10-05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TextureRecycler.h"
#include "Config.h"

#include <set>


void TextureRecycler::setSize(const uvec2& size) {
    if (size == _size) {
        return;
    }
    _size = size;
    while (!_readyTextures.empty()) {
        _readyTextures.pop();
    }
    std::set<Map::key_type> toDelete;
    std::for_each(_allTextures.begin(), _allTextures.end(), [&](Map::const_reference item) {
        if (!item.second._active && item.second._size != _size) {
            toDelete.insert(item.first);
        }
    });
    std::for_each(toDelete.begin(), toDelete.end(), [&](Map::key_type key) {
        _allTextures.erase(key);
    });
}

void TextureRecycler::clear() {
    while (!_readyTextures.empty()) {
        _readyTextures.pop();
    }
    _allTextures.clear();
}

void TextureRecycler::addTexture() {
    uint32_t newTexture;
    glGenTextures(1, &newTexture);
    glBindTexture(GL_TEXTURE_2D, newTexture);
    if (_useMipmaps) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.2f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _size.x, _size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    _allTextures.emplace(std::piecewise_construct, std::forward_as_tuple(newTexture), std::forward_as_tuple(newTexture, _size));
    _readyTextures.push(newTexture);
}

uint32_t TextureRecycler::getNextTexture() {
    while (_allTextures.size() < _textureCount) {
        addTexture();
    }

    if (_readyTextures.empty()) {
        addTexture();
    }

    uint32_t result = _readyTextures.front();
    _readyTextures.pop();
    auto& item = _allTextures[result];
    item._active = true;
    return result;
}

void TextureRecycler::recycleTexture(GLuint texture) {
    Q_ASSERT(_allTextures.count(texture));
    auto& item = _allTextures[texture];
    Q_ASSERT(item._active);
    item._active = false;
    if (item._size != _size) {
        // Buh-bye
        _allTextures.erase(texture);
        return;
    }

    _readyTextures.push(item._tex);
}

