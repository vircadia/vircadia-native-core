//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_TextureRecycler_h
#define hifi_TextureRecycler_h

#include <atomic>
#include <queue>
#include <map>

#include <GLMHelpers.h>

// GPU resources are typically buffered for one copy being used by the renderer, 
// one copy in flight, and one copy being used by the receiver
#define GPU_RESOURCE_BUFFER_SIZE 3

class TextureRecycler {
public:
    TextureRecycler(bool useMipmaps) : _useMipmaps(useMipmaps) {}
    void setSize(const uvec2& size);
    void setTextureCount(uint8_t textureCount);
    void clear();
    uint32_t getNextTexture();
    void recycleTexture(uint32_t texture);

private:
    void addTexture();

    struct TexInfo {
        const uint32_t _tex{ 0 };
        const uvec2 _size;
        bool _active { false };

        TexInfo() {}
        TexInfo(uint32_t tex, const uvec2& size) : _tex(tex), _size(size) {}
        TexInfo(const TexInfo& other) : _tex(other._tex), _size(other._size) {}
    };

    using Map = std::map<uint32_t, TexInfo>;
    using Queue = std::queue<uint32_t>;

    Map _allTextures;
    Queue _readyTextures;
    uvec2 _size{ 1920, 1080 };
    bool _useMipmaps;
    uint8_t _textureCount { GPU_RESOURCE_BUFFER_SIZE };
};

#endif
