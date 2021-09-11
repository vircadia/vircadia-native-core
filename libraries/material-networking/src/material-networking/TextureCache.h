//
//  TextureCache.h
//  libraries/model-networking/src
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextureCache_h
#define hifi_TextureCache_h

#include <gpu/Texture.h>

#include <QImage>
#include <QMap>
#include <QColor>
#include <QMetaEnum>
#include <QtCore/QSharedPointer>

#include <DependencyManager.h>
#include <ResourceCache.h>
#include <graphics/TextureMap.h>
#include <image/ColorChannel.h>
#include <image/TextureProcessing.h>
#include <ktx/KTX.h>
#include <TextureMeta.h>

#include <gpu/Context.h>
#include "KTXCache.h"

namespace gpu {
class Batch;
}

/// A simple object wrapper for an OpenGL texture.
class Texture {
public:
    gpu::TexturePointer getGPUTexture() const { return _textureSource->getGPUTexture(); }
    gpu::TextureSourcePointer _textureSource;

    static std::function<gpu::TexturePointer()> getTextureForUUIDOperator(const QUuid& uuid);
    static void setUnboundTextureForUUIDOperator(std::function<gpu::TexturePointer(const QUuid&)> textureForUUIDOperator);

private:
    static std::function<gpu::TexturePointer(const QUuid&)> _unboundTextureForUUIDOperator;
};

/// A texture loaded from the network.
class NetworkTexture : public Resource, public Texture {
    Q_OBJECT

public:
    NetworkTexture(const QUrl& url, bool resourceTexture = false);
    NetworkTexture(const NetworkTexture& other);
    ~NetworkTexture() override;

    QString getType() const override { return "NetworkTexture"; }

    int getOriginalWidth() const { return _textureSource->getGPUTexture() ? _textureSource->getGPUTexture()->getOriginalWidth() : 0; }
    int getOriginalHeight() const { return _textureSource->getGPUTexture() ? _textureSource->getGPUTexture()->getOriginalHeight() : 0; }
    int getWidth() const { return _textureSource->getGPUTexture() ? _textureSource->getGPUTexture()->getWidth() : 0; }
    int getHeight() const { return _textureSource->getGPUTexture() ? _textureSource->getGPUTexture()->getHeight() : 0; }
    image::TextureUsage::Type getTextureType() const { return _type; }

    gpu::TexturePointer getFallbackTexture() const;

    void refresh() override;

    Q_INVOKABLE void setOriginalDescriptor(ktx::KTXDescriptor* descriptor) { _originalKtxDescriptor.reset(descriptor); }

    void setExtra(void* extra) override;

signals:
    void networkTextureCreated(const QWeakPointer<NetworkTexture>& self);

public slots:
    void ktxInitialDataRequestFinished();
    void ktxMipRequestFinished();

protected:
    void makeRequest() override;
    void makeLocalRequest();
    Q_INVOKABLE void handleLocalRequestCompleted();

    Q_INVOKABLE virtual void downloadFinished(const QByteArray& data) override;

    bool handleFailedRequest(ResourceRequest::Result result) override;

    Q_INVOKABLE void loadMetaContent(const QByteArray& content);
    Q_INVOKABLE void loadTextureContent(const QByteArray& content);

    Q_INVOKABLE void setImage(gpu::TexturePointer texture, int originalWidth, int originalHeight);
    void setImageOperator(std::function<gpu::TexturePointer()> textureOperator);

    Q_INVOKABLE void startRequestForNextMipLevel();

    void startMipRangeRequest(uint16_t low, uint16_t high);
    void handleFinishedInitialLoad();

private:
    friend class KTXReader;
    friend class ImageReader;

    image::TextureUsage::Type _type { image::TextureUsage::UNUSED_TEXTURE };
    image::ColorChannel _sourceChannel;

    enum class ResourceType {
        META,
        ORIGINAL,
        KTX
    };

    ResourceType _currentlyLoadingResourceType { ResourceType::META };

    static const uint16_t NULL_MIP_LEVEL;
    enum KTXResourceState {
        PENDING_INITIAL_LOAD = 0,
        LOADING_INITIAL_DATA,    // Loading KTX Header + Low Resolution Mips
        WAITING_FOR_MIP_REQUEST, // Waiting for the gpu layer to report that it needs higher resolution mips
        PENDING_MIP_REQUEST,     // We have added ourselves to the ResourceCache queue
        REQUESTING_MIP,          // We have a mip in flight
        FAILED_TO_LOAD
    };

    KTXResourceState _ktxResourceState { PENDING_INITIAL_LOAD };

    // The current mips that are currently being requested w/ _ktxMipRequest
    std::pair<uint16_t, uint16_t> _ktxMipLevelRangeInFlight{ NULL_MIP_LEVEL, NULL_MIP_LEVEL };

    ResourceRequest* _ktxHeaderRequest { nullptr };
    ResourceRequest* _ktxMipRequest { nullptr };
    QByteArray _ktxHeaderData;
    QByteArray _ktxHighMipData;

    uint16_t _lowestRequestedMipLevel { NULL_MIP_LEVEL };
    uint16_t _lowestKnownPopulatedMip { NULL_MIP_LEVEL };

    // This is a copy of the original KTX descriptor from the source url.
    // We need this because the KTX that will be cached will likely include extra data
    // in its key/value data, and so will not match up with the original, causing
    // mip offsets to change.
    ktx::KTXDescriptorPointer _originalKtxDescriptor;

    int _width { 0 };
    int _height { 0 };
    int _maxNumPixels { ABSOLUTE_MAX_TEXTURE_NUM_PIXELS };
    QByteArray _content;

    friend class TextureCache;
};

using NetworkTexturePointer = QSharedPointer<NetworkTexture>;

Q_DECLARE_METATYPE(QWeakPointer<NetworkTexture>)


/// Stores cached textures, including render-to-texture targets.
class TextureCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:

    /// Returns the ID of the permutation/normal texture used for Perlin noise shader programs.  This texture
    /// has two lines: the first, a set of random numbers in [0, 255] to be used as permutation offsets, and
    /// the second, a set of random unit vectors to be used as noise gradients.
    const gpu::TexturePointer& getPermutationNormalTexture();

    /// Returns an opaque white texture (useful for a default).
    const gpu::TexturePointer& getWhiteTexture();

    /// Returns an opaque gray texture (useful for a default).
    const gpu::TexturePointer& getGrayTexture();

    /// Returns the a pale blue texture (useful for a normal map).
    const gpu::TexturePointer& getBlueTexture();

    /// Returns the a black texture (useful for a default).
    const gpu::TexturePointer& getBlackTexture();

    /// Returns a texture version of an image file
    static gpu::TexturePointer getImageTexture(const QString& path, image::TextureUsage::Type type = image::TextureUsage::DEFAULT_TEXTURE);

    /// Loads a texture from the specified URL.
    NetworkTexturePointer getTexture(const QUrl& url, image::TextureUsage::Type type = image::TextureUsage::DEFAULT_TEXTURE,
        const QByteArray& content = QByteArray(), int maxNumPixels = ABSOLUTE_MAX_TEXTURE_NUM_PIXELS,
        image::ColorChannel sourceChannel = image::ColorChannel::NONE);

    std::pair<gpu::TexturePointer, glm::ivec2> getTextureByHash(const std::string& hash);
    std::pair<gpu::TexturePointer, glm::ivec2> cacheTextureByHash(const std::string& hash, const std::pair<gpu::TexturePointer, glm::ivec2>& textureAndSize);

    NetworkTexturePointer getResourceTexture(const QUrl& resourceTextureUrl);
    const gpu::FramebufferPointer& getHmdPreviewFramebuffer(int width, int height);
    const gpu::FramebufferPointer& getSpectatorCameraFramebuffer();
    const gpu::FramebufferPointer& getSpectatorCameraFramebuffer(int width, int height);
    void updateSpectatorCameraNetworkTexture();

    NetworkTexturePointer getTextureByUUID(const QString& uuid);

    static const int DEFAULT_SPECTATOR_CAM_WIDTH { 2048 };
    static const int DEFAULT_SPECTATOR_CAM_HEIGHT { 1024 };

    void setGPUContext(const gpu::ContextPointer& context) { _gpuContext = context; }
    gpu::ContextPointer getGPUContext() const { return _gpuContext; }

signals:
    void spectatorCameraFramebufferReset();

protected:
    
    // Overload ResourceCache::prefetch to allow specifying texture type for loads
    Q_INVOKABLE ScriptableResource* prefetch(const QUrl& url, int type, int maxNumPixels = ABSOLUTE_MAX_TEXTURE_NUM_PIXELS, image::ColorChannel sourceChannel = image::ColorChannel::NONE);

    virtual QSharedPointer<Resource> createResource(const QUrl& url) override;
    QSharedPointer<Resource> createResourceCopy(const QSharedPointer<Resource>& resource) override;

private:
    friend class ImageReader;
    friend class NetworkTexture;
    friend class DilatableNetworkTexture;
    friend class TextureCacheScriptingInterface;

    TextureCache();
    virtual ~TextureCache();

    static const std::string KTX_DIRNAME;
    static const std::string KTX_EXT;

    gpu::ContextPointer _gpuContext { nullptr };

    std::shared_ptr<cache::FileCache> _ktxCache { std::make_shared<KTXCache>(KTX_DIRNAME, KTX_EXT) };

    // Map from image hashes to texture weak pointers
    std::unordered_map<std::string, std::pair<std::weak_ptr<gpu::Texture>, glm::ivec2>> _texturesByHashes;
    std::mutex _texturesByHashesMutex;

    gpu::TexturePointer _permutationNormalTexture;
    gpu::TexturePointer _whiteTexture;
    gpu::TexturePointer _grayTexture;
    gpu::TexturePointer _blueTexture;
    gpu::TexturePointer _blackTexture;

    NetworkTexturePointer _spectatorCameraNetworkTexture;
    gpu::FramebufferPointer _spectatorCameraFramebuffer;

    NetworkTexturePointer _hmdPreviewNetworkTexture;
    gpu::FramebufferPointer _hmdPreviewFramebuffer;
};

#endif // hifi_TextureCache_h
