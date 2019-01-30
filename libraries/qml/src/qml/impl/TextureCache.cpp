#include "TextureCache.h"

#include <cassert>

#include <gl/Config.h>

#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Profiling.h"

using namespace hifi::qml::impl;

uint64_t uvec2ToUint64(const QSize& size) {
    uint64_t result = size.width();
    result <<= 32;
    result |= size.height();
    return result;
}

void TextureCache::acquireSize(const QSize& size) {
    auto sizeKey = uvec2ToUint64(size);
    Lock lock(_mutex);
    auto& textureSet = _textures[sizeKey];
    ++textureSet.clientCount;
}

void TextureCache::releaseSize(const QSize& size) {
    auto sizeKey = uvec2ToUint64(size);
    {
        Lock lock(_mutex);
        assert(_textures.count(sizeKey));
        auto& textureSet = _textures[sizeKey];
        if (0 == --textureSet.clientCount) {
            for (const auto& textureAndFence : textureSet.returnedTextures) {
                destroy(textureAndFence);
            }
            _textures.erase(sizeKey);
        }
    }
}

uint32_t TextureCache::acquireTexture(const QSize& size) {
    Lock lock(_mutex);
    recycle();

    ++_activeTextureCount;
    auto sizeKey = uvec2ToUint64(size);
    assert(_textures.count(sizeKey));
    auto& textureSet = _textures[sizeKey];
    if (!textureSet.returnedTextures.empty()) {
        auto textureAndFence = textureSet.returnedTextures.front();
        textureSet.returnedTextures.pop_front();
        if (textureAndFence.second) {
            glWaitSync((GLsync)textureAndFence.second, 0, GL_TIMEOUT_IGNORED);
            glDeleteSync((GLsync)textureAndFence.second);
        }
        return textureAndFence.first;
    }
    return createTexture(size);
}

void TextureCache::releaseTexture(const Value& textureAndFence) {
    --_activeTextureCount;
    Lock lock(_mutex);
    _returnedTextures.push_back(textureAndFence);
}

void TextureCache::report() {
    if (randFloat() < 0.01f) {
        PROFILE_COUNTER(render_qml_gl, "offscreenTextures",
                        {
                            { "total", QVariant::fromValue(_allTextureCount.load()) },
                            { "active", QVariant::fromValue(_activeTextureCount.load()) },
                        });
        PROFILE_COUNTER(render_qml_gl, "offscreenTextureMemory", { { "value", QVariant::fromValue(_totalTextureUsage) } });
    }
}

size_t TextureCache::getUsedTextureMemory() {
    size_t toReturn;
    {
        Lock lock(_mutex);
        toReturn = _totalTextureUsage;
    }
    return toReturn;
}

size_t TextureCache::getMemoryForSize(const QSize& size) {
    // Base size + mips
    return static_cast<size_t>(((size.width() * size.height()) << 2) * 1.33f);
}

void TextureCache::destroyTexture(uint32_t texture) {
    --_allTextureCount;
    auto size = _textureSizes[texture];
    assert(getMemoryForSize(size) <= _totalTextureUsage);
    _totalTextureUsage -= getMemoryForSize(size);
    _textureSizes.erase(texture);
    // FIXME prevents crash on shutdown, but we should migrate to a global functions object owned by the shared context.
    glDeleteTextures(1, &texture);
}

void TextureCache::destroy(const Value& textureAndFence) {
    const auto& fence = textureAndFence.second;
    if (fence) {
        // FIXME prevents crash on shutdown, but we should migrate to a global functions object owned by the shared context.
        glWaitSync((GLsync)fence, 0, GL_TIMEOUT_IGNORED);
        glDeleteSync((GLsync)fence);
    }
    destroyTexture(textureAndFence.first);
}

uint32_t TextureCache::createTexture(const QSize& size) {
    // Need a new texture
    uint32_t newTexture;
    glGenTextures(1, &newTexture);
    ++_allTextureCount;
    _textureSizes[newTexture] = size;
    _totalTextureUsage += getMemoryForSize(size);
    glBindTexture(GL_TEXTURE_2D, newTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#if !defined(USE_GLES)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.2f);
#endif
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.width(), size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    return newTexture;
}

void TextureCache::recycle() {
    // First handle any global returns
    ValueList returnedTextures;
    returnedTextures.swap(_returnedTextures);

    for (auto textureAndFence : returnedTextures) {
        GLuint texture = textureAndFence.first;
        QSize size = _textureSizes[texture];
        auto sizeKey = uvec2ToUint64(size);
        // Textures can be returned after all surfaces of the given size have been destroyed,
        // in which case we just destroy the texture
        if (!_textures.count(sizeKey)) {
            destroy(textureAndFence);
            continue;
        }
        _textures[sizeKey].returnedTextures.push_back(textureAndFence);
    }
}
