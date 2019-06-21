//
//  Created by Bradley Austin Davis on 2018-01-04
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_QmlTextureCache_h
#define hifi_QmlTextureCache_h

#include <list>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <utility>
#include <cstdint>

#include <QtCore/QSize>

namespace hifi { namespace qml { namespace impl {

class TextureAndFence : public std::pair<uint32_t, void*> {
    using Parent = std::pair<uint32_t, void*>;
public:
    TextureAndFence() : Parent(0, 0) {}
    TextureAndFence(uint32_t texture, void* sync) : Parent(texture, sync) {};
};


class TextureCache {
public:
    using Value = TextureAndFence;
    using ValueList = std::list<Value>;
    using Size = uint64_t;

    struct TextureSet {
        // The number of surfaces with this size
        size_t clientCount { 0 };
        ValueList returnedTextures;
    };

    void releaseSize(const QSize& size);
    void acquireSize(const QSize& size);
    uint32_t acquireTexture(const QSize& size);
    void releaseTexture(const Value& textureAndFence);

    // For debugging
    void report();
    size_t getUsedTextureMemory();
private:
    static size_t getMemoryForSize(const QSize& size);

    uint32_t createTexture(const QSize& size);
    void destroyTexture(uint32_t texture);

    void destroy(const Value& textureAndFence);
    void recycle();

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    std::atomic<int> _allTextureCount;
    std::atomic<int> _activeTextureCount;
    std::unordered_map<Size, TextureSet> _textures;
    std::unordered_map<uint32_t, QSize> _textureSizes;
    Mutex _mutex;
    std::list<Value> _returnedTextures;
    size_t _totalTextureUsage { 0 };
};

}}}  // namespace hifi::qml::impl

#endif
