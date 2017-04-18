//
//  TextureCache.cpp
//  libraries/model-networking/src
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextureCache.h"

#include <mutex>

#include <QCryptographicHash>
#include <QImageReader>
#include <QRunnable>
#include <QThreadPool>
#include <QNetworkReply>
#include <QPainter>

#if DEBUG_DUMP_TEXTURE_LOADS
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include <gpu/Batch.h>

#include <image/Image.h>

#include <NumericalConstants.h>
#include <shared/NsightHelpers.h>

#include <Finally.h>
#include <Profile.h>

#include "NetworkLogging.h"
#include "ModelNetworkingLogging.h"
#include <Trace.h>
#include <StatTracker.h>

Q_LOGGING_CATEGORY(trace_resource_parse_image, "trace.resource.parse.image")
Q_LOGGING_CATEGORY(trace_resource_parse_image_raw, "trace.resource.parse.image.raw")
Q_LOGGING_CATEGORY(trace_resource_parse_image_ktx, "trace.resource.parse.image.ktx")

const std::string TextureCache::KTX_DIRNAME { "ktx_cache" };
const std::string TextureCache::KTX_EXT { "ktx" };

TextureCache::TextureCache() :
    _ktxCache(KTX_DIRNAME, KTX_EXT) {
    setUnusedResourceCacheSize(0);
    setObjectName("TextureCache");
}

TextureCache::~TextureCache() {
}

// use fixed table of permutations. Could also make ordered list programmatically
// and then shuffle algorithm. For testing, this ensures consistent behavior in each run.
// this list taken from Ken Perlin's Improved Noise reference implementation (orig. in Java) at
// http://mrl.nyu.edu/~perlin/noise/

const int permutation[256] =
{
    151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
    140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
    247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
     57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
     74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
     60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
     65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
     200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186,   3,  64,
     52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
    207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
    119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
    129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
    218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
     81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
    184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
    222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180
};

#define USE_CHRIS_NOISE 1

const gpu::TexturePointer& TextureCache::getPermutationNormalTexture() {
    if (!_permutationNormalTexture) {

        // the first line consists of random permutation offsets
        unsigned char data[256 * 2 * 3];
#if (USE_CHRIS_NOISE==1)
        for (int i = 0; i < 256; i++) {
            data[3*i+0] = permutation[i];
            data[3*i+1] = permutation[i];
            data[3*i+2] = permutation[i];
        }
#else
        for (int i = 0; i < 256 * 3; i++) {
            data[i] = rand() % 256;
        }
#endif

        for (int i = 256 * 3; i < 256 * 3 * 2; i += 3) {
            glm::vec3 randvec = glm::sphericalRand(1.0f);
            data[i] = ((randvec.x + 1.0f) / 2.0f) * 255.0f;
            data[i + 1] = ((randvec.y + 1.0f) / 2.0f) * 255.0f;
            data[i + 2] = ((randvec.z + 1.0f) / 2.0f) * 255.0f;
        }

        _permutationNormalTexture = gpu::Texture::create2D(gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB), 256, 2);
        _permutationNormalTexture->setStoredMipFormat(_permutationNormalTexture->getTexelFormat());
        _permutationNormalTexture->assignStoredMip(0, sizeof(data), data);
    }
    return _permutationNormalTexture;
}

const unsigned char OPAQUE_WHITE[] = { 0xFF, 0xFF, 0xFF, 0xFF };
const unsigned char OPAQUE_GRAY[] = { 0x80, 0x80, 0x80, 0xFF };
const unsigned char OPAQUE_BLUE[] = { 0x80, 0x80, 0xFF, 0xFF };
const unsigned char OPAQUE_BLACK[] = { 0x00, 0x00, 0x00, 0xFF };

const gpu::TexturePointer& TextureCache::getWhiteTexture() {
    if (!_whiteTexture) {
        _whiteTexture = gpu::Texture::createStrict(gpu::Element::COLOR_RGBA_32, 1, 1);
        _whiteTexture->setSource("TextureCache::_whiteTexture");
        _whiteTexture->setStoredMipFormat(_whiteTexture->getTexelFormat());
        _whiteTexture->assignStoredMip(0, sizeof(OPAQUE_WHITE), OPAQUE_WHITE);
    }
    return _whiteTexture;
}

const gpu::TexturePointer& TextureCache::getGrayTexture() {
    if (!_grayTexture) {
        _grayTexture = gpu::Texture::createStrict(gpu::Element::COLOR_RGBA_32, 1, 1);
        _grayTexture->setSource("TextureCache::_grayTexture");
        _grayTexture->setStoredMipFormat(_grayTexture->getTexelFormat());
        _grayTexture->assignStoredMip(0, sizeof(OPAQUE_GRAY), OPAQUE_GRAY);
    }
    return _grayTexture;
}

const gpu::TexturePointer& TextureCache::getBlueTexture() {
    if (!_blueTexture) {
        _blueTexture = gpu::Texture::createStrict(gpu::Element::COLOR_RGBA_32, 1, 1);
        _blueTexture->setSource("TextureCache::_blueTexture");
        _blueTexture->setStoredMipFormat(_blueTexture->getTexelFormat());
        _blueTexture->assignStoredMip(0, sizeof(OPAQUE_BLUE), OPAQUE_BLUE);
    }
    return _blueTexture;
}

const gpu::TexturePointer& TextureCache::getBlackTexture() {
    if (!_blackTexture) {
        _blackTexture = gpu::Texture::createStrict(gpu::Element::COLOR_RGBA_32, 1, 1);
        _blackTexture->setSource("TextureCache::_blackTexture");
        _blackTexture->setStoredMipFormat(_blackTexture->getTexelFormat());
        _blackTexture->assignStoredMip(0, sizeof(OPAQUE_BLACK), OPAQUE_BLACK);
    }
    return _blackTexture;
}

/// Extra data for creating textures.
class TextureExtra {
public:
    image::TextureUsage::Type type;
    const QByteArray& content;
    int maxNumPixels;
};

ScriptableResource* TextureCache::prefetch(const QUrl& url, int type, int maxNumPixels) {
    auto byteArray = QByteArray();
    TextureExtra extra = { (image::TextureUsage::Type)type, byteArray, maxNumPixels };
    return ResourceCache::prefetch(url, &extra);
}

NetworkTexturePointer TextureCache::getTexture(const QUrl& url, image::TextureUsage::Type type, const QByteArray& content, int maxNumPixels) {
    TextureExtra extra = { type, content, maxNumPixels };
    return ResourceCache::getResource(url, QUrl(), &extra).staticCast<NetworkTexture>();
}

gpu::TexturePointer TextureCache::getTextureByHash(const std::string& hash) {
    std::weak_ptr<gpu::Texture> weakPointer;
    {
        std::unique_lock<std::mutex> lock(_texturesByHashesMutex);
        weakPointer = _texturesByHashes[hash];
    }
    auto result = weakPointer.lock();
    if (result) {
        qCWarning(modelnetworking) << "QQQ Returning live texture for hash " << hash.c_str();
    }
    return result;
}

gpu::TexturePointer TextureCache::cacheTextureByHash(const std::string& hash, const gpu::TexturePointer& texture) {
    gpu::TexturePointer result;
    {
        std::unique_lock<std::mutex> lock(_texturesByHashesMutex);
        result = _texturesByHashes[hash].lock();
        if (!result) {
            _texturesByHashes[hash] = texture;
            result = texture;
        } else {
            qCWarning(modelnetworking) << "QQQ Swapping out texture with previous live texture in hash " << hash.c_str();
        }
    }
    return result;
}

gpu::TexturePointer getFallbackTextureForType(image::TextureUsage::Type type) {
    gpu::TexturePointer result;
    auto textureCache = DependencyManager::get<TextureCache>();
    // Since this can be called on a background thread, there's a chance that the cache 
    // will be destroyed by the time we request it
    if (!textureCache) {
        return result;
    }
    switch (type) {
        case image::TextureUsage::DEFAULT_TEXTURE:
        case image::TextureUsage::ALBEDO_TEXTURE:
        case image::TextureUsage::ROUGHNESS_TEXTURE:
        case image::TextureUsage::OCCLUSION_TEXTURE:
            result = textureCache->getWhiteTexture();
            break;

        case image::TextureUsage::NORMAL_TEXTURE:
            result = textureCache->getBlueTexture();
            break;

        case image::TextureUsage::EMISSIVE_TEXTURE:
        case image::TextureUsage::LIGHTMAP_TEXTURE:
            result = textureCache->getBlackTexture();
            break;

        case image::TextureUsage::BUMP_TEXTURE:
        case image::TextureUsage::SPECULAR_TEXTURE:
        case image::TextureUsage::GLOSS_TEXTURE:
        case image::TextureUsage::CUBE_TEXTURE:
        case image::TextureUsage::STRICT_TEXTURE:
        default:
            break;
    }
    return result;
}

NetworkTexture::~NetworkTexture() {
    auto texture = _textureSource->getGPUTexture();
    if (texture) {
        texture->unregisterMipInterestListener(this);
    }
}

/// Returns a texture version of an image file
gpu::TexturePointer TextureCache::getImageTexture(const QString& path, image::TextureUsage::Type type, QVariantMap options) {
    QImage image = QImage(path);
    auto loader = image::TextureUsage::getTextureLoaderForType(type, options);
    return gpu::TexturePointer(loader(image, QUrl::fromLocalFile(path).fileName().toStdString()));
}

QSharedPointer<Resource> TextureCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
    const void* extra) {
    const TextureExtra* textureExtra = static_cast<const TextureExtra*>(extra);
    auto type = textureExtra ? textureExtra->type : image::TextureUsage::DEFAULT_TEXTURE;
    auto content = textureExtra ? textureExtra->content : QByteArray();
    auto maxNumPixels = textureExtra ? textureExtra->maxNumPixels : ABSOLUTE_MAX_TEXTURE_NUM_PIXELS;
    NetworkTexture* texture = new NetworkTexture(url, type, content, maxNumPixels);
    return QSharedPointer<Resource>(texture, &Resource::deleter);
}

NetworkTexture::NetworkTexture(const QUrl& url, image::TextureUsage::Type type, const QByteArray& content, int maxNumPixels) :
    Resource(url),
    _type(type),
    _maxNumPixels(maxNumPixels),
    _sourceIsKTX(url.path().endsWith(".ktx"))
{
    _textureSource = std::make_shared<gpu::TextureSource>();

    if (!url.isValid()) {
        _loaded = true;
    }

    if (_sourceIsKTX) {
        _requestByteRange.fromInclusive = 0;
        _requestByteRange.toExclusive = 1000;
    }

    // if we have content, load it after we have our self pointer
    if (!content.isEmpty()) {
        _startedLoading = true;
        QMetaObject::invokeMethod(this, "loadContent", Qt::QueuedConnection, Q_ARG(const QByteArray&, content));
    }
}

void NetworkTexture::setImage(gpu::TexturePointer texture, int originalWidth,
                              int originalHeight) {
    _originalWidth = originalWidth;
    _originalHeight = originalHeight;

    // Passing ownership
    _textureSource->resetTexture(texture);

    if (texture) {
        _width = texture->getWidth();
        _height = texture->getHeight();
        setSize(texture->getStoredSize());
    } else {
        // FIXME: If !gpuTexture, we failed to load!
        _width = _height = 0;
        qWarning() << "Texture did not load";
    }

    finishedLoading(true);

    emit networkTextureCreated(qWeakPointerCast<NetworkTexture, Resource> (_self));
}

gpu::TexturePointer NetworkTexture::getFallbackTexture() const {
    return getFallbackTextureForType(_type);
}

class ImageReader : public QRunnable {
public:
    ImageReader(const QWeakPointer<Resource>& resource, const QUrl& url,
                const QByteArray& data, int maxNumPixels);
    void run() override final;
    void read();

private:
    static void listSupportedImageFormats();

    QWeakPointer<Resource> _resource;
    QUrl _url;
    QByteArray _content;
    int _maxNumPixels;
};

const uint16_t NetworkTexture::NULL_MIP_LEVEL = std::numeric_limits<uint16_t>::max();
void NetworkTexture::makeRequest() {
    if (!_sourceIsKTX) {
        Resource::makeRequest();
        return;
    } 

    // We special-handle ktx requests to run 2 concurrent requests right off the bat
    PROFILE_ASYNC_BEGIN(resource, "Resource:" + getType(), QString::number(_requestID), { { "url", _url.toString() }, { "activeURL", _activeUrl.toString() } });

    if (!_ktxHeaderLoaded && !_highMipRequestFinished) {
        qDebug() << ">>> Making request to " << _url << " for header";
        _ktxHeaderRequest = ResourceManager::createResourceRequest(this, _activeUrl);

        if (!_ktxHeaderRequest) {
            //qCDebug(networking).noquote() << "Failed to get request for" << _url.toDisplayString();
            PROFILE_ASYNC_END(resource, "Resource:" + getType(), QString::number(_requestID));
            return;
        }

        ByteRange range;
        range.fromInclusive = 0;
        range.toExclusive = 1000;
        _ktxHeaderRequest->setByteRange(range);

        //qCDebug(resourceLog).noquote() << "Starting request for:" << _url.toDisplayString();
        emit loading();

        connect(_ktxHeaderRequest, &ResourceRequest::progress, this, &NetworkTexture::ktxHeaderRequestProgress);
        //connect(this, &Resource::onProgress, this, &NetworkTexture::ktxHeaderRequestFinished);

        connect(_ktxHeaderRequest, &ResourceRequest::finished, this, &NetworkTexture::ktxHeaderRequestFinished);

        _bytesReceived = _bytesTotal = _bytes = 0;

        _ktxHeaderRequest->send();

        startMipRangeRequest(NULL_MIP_LEVEL, NULL_MIP_LEVEL);
    } else {
        if (_lowestKnownPopulatedMip > 0) {
            startMipRangeRequest(_lowestKnownPopulatedMip - 1, _lowestKnownPopulatedMip - 1);
        }
    }

}

void NetworkTexture::handleMipInterestCallback(uint16_t level) {
    QMetaObject::invokeMethod(this, "handleMipInterestLevel", Qt::QueuedConnection, Q_ARG(int, level));
}

void NetworkTexture::handleMipInterestLevel(int level) {
    _lowestRequestedMipLevel = std::min(static_cast<uint16_t>(level), _lowestRequestedMipLevel);
    if (!_ktxMipRequest && _lowestKnownPopulatedMip > 0) {
        //startRequestForNextMipLevel();
        clearLoadPriority(this);
        setLoadPriority(this, _lowestKnownPopulatedMip - 1);
        ResourceCache::attemptRequest(_self);
    }
}

void NetworkTexture::startRequestForNextMipLevel() {
    if (_lowestKnownPopulatedMip == 0) {
        qWarning(networking) << "Requesting next mip level but all have been fulfilled";
        return;
    }
    if (_pending) {
        return;
    }

    //startMipRangeRequest(std::max(0, _lowestKnownPopulatedMip - 1), std::max(0, _lowestKnownPopulatedMip - 1));
}

// Load mips in the range [low, high] (inclusive)
void NetworkTexture::startMipRangeRequest(uint16_t low, uint16_t high) {
    if (_ktxMipRequest) {
        return;
    }

    bool isHighMipRequest = low == NULL_MIP_LEVEL && high == NULL_MIP_LEVEL;
    
    if (!isHighMipRequest && !_ktxHeaderLoaded) {
        return;
    }

    _ktxMipRequest = ResourceManager::createResourceRequest(this, _activeUrl);
    qDebug(networking) << ">>> Making request to " << _url << " for " << low << " to " << high;

    _ktxMipLevelRangeInFlight = { low, high };
    if (isHighMipRequest) {
        // This is a special case where we load the high 7 mips
        ByteRange range;
        range.fromInclusive = -15000;
        _ktxMipRequest->setByteRange(range);
    } else {
        ByteRange range;
        range.fromInclusive = ktx::KTX_HEADER_SIZE + _originalKtxDescriptor->header.bytesOfKeyValueData
                              + _originalKtxDescriptor->images[low]._imageOffset + 4;
        range.toExclusive = ktx::KTX_HEADER_SIZE + _originalKtxDescriptor->header.bytesOfKeyValueData
                              + _originalKtxDescriptor->images[high + 1]._imageOffset;
        _ktxMipRequest->setByteRange(range);
    }

    connect(_ktxMipRequest, &ResourceRequest::progress, this, &NetworkTexture::ktxMipRequestProgress);
    connect(_ktxMipRequest, &ResourceRequest::finished, this, &NetworkTexture::ktxMipRequestFinished);

    _ktxMipRequest->send();
}


void NetworkTexture::ktxHeaderRequestFinished() {
    assert(!_ktxHeaderLoaded);

    _headerRequestFinished = true;
    if (_ktxHeaderRequest->getResult() == ResourceRequest::Success) {
        _ktxHeaderLoaded = true;
        _ktxHeaderData = _ktxHeaderRequest->getData();
        maybeHandleFinishedInitialLoad();
    } else {
        handleFailedRequest(_ktxHeaderRequest->getResult());
    }
    _ktxHeaderRequest->deleteLater();
    _ktxHeaderRequest = nullptr;
}

void NetworkTexture::ktxMipRequestFinished() {
    bool isHighMipRequest = _ktxMipLevelRangeInFlight.first == NULL_MIP_LEVEL
                            && _ktxMipLevelRangeInFlight.second == NULL_MIP_LEVEL;

    if (_ktxMipRequest->getResult() == ResourceRequest::Success) {
        if (_highMipRequestFinished) {
            assert(_ktxMipLevelRangeInFlight.second - _ktxMipLevelRangeInFlight.first == 0);

            _lowestKnownPopulatedMip = _ktxMipLevelRangeInFlight.first;
            
            auto texture = _textureSource->getGPUTexture();
            if (!texture) {
                texture->assignStoredMip(_ktxMipLevelRangeInFlight.first,
                    _ktxMipRequest->getData().size(), reinterpret_cast<uint8_t*>(_ktxMipRequest->getData().data()));
            } else {
                qWarning(networking) << "Trying to update mips but texture is null";
            }
        } else {
            _highMipRequestFinished = true;
            _ktxHighMipData = _ktxMipRequest->getData();
            maybeHandleFinishedInitialLoad();
        }
    } else {
        handleFailedRequest(_ktxMipRequest->getResult());
    }
    _ktxMipRequest->deleteLater();
    _ktxMipRequest = nullptr;
}

// This is called when the header or top mips have been loaded
void NetworkTexture::maybeHandleFinishedInitialLoad() {
    if (_headerRequestFinished && _highMipRequestFinished) {
        ResourceCache::requestCompleted(_self);

        if (_ktxHeaderData.size() == 0 || _ktxHighMipData.size() == 0) {
            finishedLoading(false);
        } else {
            // create ktx...
            auto header = reinterpret_cast<const ktx::Header*>(_ktxHeaderData.data());

            qDebug() << "Creating KTX";
            qDebug() << "Identifier:" << QString(QByteArray((char*)header->identifier, 12));
            qDebug() << "Type:" << header->glType;
            qDebug() << "TypeSize:" << header->glTypeSize;
            qDebug() << "numberOfArrayElements:" << header->numberOfArrayElements;
            qDebug() << "numberOfFaces:" << header->numberOfFaces;
            qDebug() << "numberOfMipmapLevels:" << header->numberOfMipmapLevels;
            auto kvSize = header->bytesOfKeyValueData;
            if (kvSize > _ktxHeaderData.size() - ktx::KTX_HEADER_SIZE) {
                qWarning() << "Cannot load " << _url << ", did not receive all kv data with initial request";
                finishedLoading(false);
                return;
            }

            auto keyValues = ktx::KTX::parseKeyValues(header->bytesOfKeyValueData, reinterpret_cast<const ktx::Byte*>(_ktxHeaderData.data()) + ktx::KTX_HEADER_SIZE);

            auto imageDescriptors = header->generateImageDescriptors();
            _originalKtxDescriptor.reset(new ktx::KTXDescriptor(*header, keyValues, imageDescriptors));

            // Create bare ktx in memory
            auto found = std::find_if(keyValues.begin(), keyValues.end(), [](const ktx::KeyValue& val) -> bool {
                return val._key.compare(gpu::SOURCE_HASH_KEY) == 0;
            });
            std::string filename;
            if (found == keyValues.end()) {
                qWarning("Source hash key not found, bailing");
                finishedLoading(false);
                return;
            }
            else {
                if (found->_value.size() < 32) {
                    qWarning("Invalid source hash key found, bailing");
                    finishedLoading(false);
                    return;
                } else {
                    filename = std::string(reinterpret_cast<char*>(found->_value.data()), 32);
                }
            }

            auto memKtx = ktx::KTX::createBare(*header, keyValues);

            //auto d = const_cast<uint8_t*>(memKtx->getStorage()->data());
            //memcpy(d + memKtx->_storage->size() - _ktxHighMipData.size(), _ktxHighMipData.data(), _ktxHighMipData.size());

            auto textureCache = DependencyManager::get<TextureCache>();

            // Move ktx to file
            const char* data = reinterpret_cast<const char*>(memKtx->_storage->data());
            size_t length = memKtx->_storage->size();
            KTXFilePointer file;
            auto& ktxCache = textureCache->_ktxCache;
            if (!memKtx || !(file = ktxCache.writeFile(data, KTXCache::Metadata(filename, length)))) {
                qCWarning(modelnetworking) << _url << " failed to write cache file";
                finishedLoading(false);
                return;
            }
            else {
                _file = file;
            }

            //_ktxDescriptor.reset(new ktx::KTXDescriptor(memKtx->toDescriptor()));
            auto newKtxDescriptor = memKtx->toDescriptor();

            //auto texture = gpu::Texture::serializeHeader("test.ktx", *header, keyValues);
            gpu::TexturePointer texture;
            texture.reset(gpu::Texture::unserialize(_file->getFilepath(), newKtxDescriptor));
            texture->setKtxBacking(file->getFilepath());
            texture->setSource(filename);
            texture->registerMipInterestListener(this);

            auto& images = _originalKtxDescriptor->images;
            size_t imageSizeRemaining = _ktxHighMipData.size();
            uint8_t* ktxData = reinterpret_cast<uint8_t*>(_ktxHighMipData.data());
            ktxData += _ktxHighMipData.size();
            // TODO Move image offset calculation to ktx ImageDescriptor
            int level;
            for (level = images.size() - 1; level >= 0; --level) {
                auto& image = images[level];
                if (image._imageSize > imageSizeRemaining) {
                    break;
                }
                qDebug() << "Transferring " << level;
                ktxData -= image._imageSize;
                texture->assignStoredMip(level, image._imageSize, ktxData);
                ktxData -= 4;
                imageSizeRemaining - image._imageSize - 4;
            }

            // We replace the texture with the one stored in the cache.  This deals with the possible race condition of two different 
            // images with the same hash being loaded concurrently.  Only one of them will make it into the cache by hash first and will
            // be the winner
            if (textureCache) {
                texture = textureCache->cacheTextureByHash(filename, texture);
            }


            _lowestKnownPopulatedMip = _originalKtxDescriptor->header.numberOfMipmapLevels;
            for (uint16_t l = 0; l < 200; l++) {
                if (texture->isStoredMipFaceAvailable(l)) {
                    _lowestKnownPopulatedMip = l;
                    break;
                }
            }

            setImage(texture, header->getPixelWidth(), header->getPixelHeight());
        }
    }
}

void NetworkTexture::downloadFinished(const QByteArray& data) {
    loadContent(data);
}

void NetworkTexture::loadContent(const QByteArray& content) {
    if (_sourceIsKTX) {
        assert(false);
        return;
    }

    QThreadPool::globalInstance()->start(new ImageReader(_self, _url, content, _maxNumPixels));
}

ImageReader::ImageReader(const QWeakPointer<Resource>& resource, const QUrl& url, const QByteArray& data, int maxNumPixels) :
    _resource(resource),
    _url(url),
    _content(data),
    _maxNumPixels(maxNumPixels)
{
    DependencyManager::get<StatTracker>()->incrementStat("PendingProcessing");
    listSupportedImageFormats();

#if DEBUG_DUMP_TEXTURE_LOADS
    static auto start = usecTimestampNow() / USECS_PER_MSEC;
    auto now = usecTimestampNow() / USECS_PER_MSEC - start;
    QString urlStr = _url.toString();
    auto dot = urlStr.lastIndexOf(".");
    QString outFileName = QString(QCryptographicHash::hash(urlStr.toLocal8Bit(), QCryptographicHash::Md5).toHex()) + urlStr.right(urlStr.length() - dot);
    QFile loadRecord("h:/textures/loads.txt");
    loadRecord.open(QFile::Text | QFile::Append | QFile::ReadWrite);
    loadRecord.write(QString("%1 %2\n").arg(now).arg(outFileName).toLocal8Bit());
    outFileName = "h:/textures/" + outFileName;
    QFileInfo outInfo(outFileName);
    if (!outInfo.exists()) {
        QFile outFile(outFileName);
        outFile.open(QFile::WriteOnly | QFile::Truncate);
        outFile.write(data);
        outFile.close();
    }
#endif
}

void ImageReader::listSupportedImageFormats() {
    static std::once_flag once;
    std::call_once(once, []{
        auto supportedFormats = QImageReader::supportedImageFormats();
        qCDebug(modelnetworking) << "List of supported Image formats:" << supportedFormats.join(", ");
    });
}

void ImageReader::run() {
    PROFILE_RANGE_EX(resource_parse_image, __FUNCTION__, 0xffff0000, 0, { { "url", _url.toString() } });
    DependencyManager::get<StatTracker>()->decrementStat("PendingProcessing");
    CounterStat counter("Processing");

    auto originalPriority = QThread::currentThread()->priority();
    if (originalPriority == QThread::InheritPriority) {
        originalPriority = QThread::NormalPriority;
    }
    QThread::currentThread()->setPriority(QThread::LowPriority);
    Finally restorePriority([originalPriority] { QThread::currentThread()->setPriority(originalPriority); });

    read();
}

void ImageReader::read() {
    auto resource = _resource.lock(); // to ensure the resource is still needed
    if (!resource) {
        qCWarning(modelnetworking) << "Abandoning load of" << _url << "; could not get strong ref";
        return;
    }
    auto networkTexture = resource.staticCast<NetworkTexture>();

    // Hash the source image to for KTX caching
    std::string hash;
    {
        QCryptographicHash hasher(QCryptographicHash::Md5);
        hasher.addData(_content);
        hash = hasher.result().toHex().toStdString();
    }

    // Maybe load from cache
    auto textureCache = DependencyManager::get<TextureCache>();
    if (textureCache) {
        // If we already have a live texture with the same hash, use it
        auto texture = textureCache->getTextureByHash(hash);

        // If there is no live texture, check if there's an existing KTX file
        if (!texture) {
            KTXFilePointer ktxFile = textureCache->_ktxCache.getFile(hash);
            if (ktxFile) {
                texture = gpu::Texture::unserialize(ktxFile->getFilepath());
                if (texture) {
                    texture = textureCache->cacheTextureByHash(hash, texture);
                }
            }
        }

        // If we found the texture either because it's in use or via KTX deserialization, 
        // set the image and return immediately.
        if (texture) {
            QMetaObject::invokeMethod(resource.data(), "setImage",
                                      Q_ARG(gpu::TexturePointer, texture),
                                      Q_ARG(int, texture->getWidth()),
                                      Q_ARG(int, texture->getHeight()));
            return;
        }
    }

    // Proccess new texture
    gpu::TexturePointer texture;
    {
        PROFILE_RANGE_EX(resource_parse_image_raw, __FUNCTION__, 0xffff0000, 0);
        texture = image::processImage(_content, _url.toString().toStdString(), _maxNumPixels, networkTexture->getTextureType());

        if (!texture) {
            qCWarning(modelnetworking) << "Could not process:" << _url;
            QMetaObject::invokeMethod(resource.data(), "setImage",
                                      Q_ARG(gpu::TexturePointer, texture),
                                      Q_ARG(int, 0),
                                      Q_ARG(int, 0));
            return;
        }

        texture->setSourceHash(hash);
        texture->setFallbackTexture(networkTexture->getFallbackTexture());
    }

    // Save the image into a KTXFile
    if (texture && textureCache) {
        auto memKtx = gpu::Texture::serialize(*texture);

        // Move the texture into a memory mapped file
        if (memKtx) {
            const char* data = reinterpret_cast<const char*>(memKtx->_storage->data());
            size_t length = memKtx->_storage->size();
            auto& ktxCache = textureCache->_ktxCache;
            networkTexture->_file = ktxCache.writeFile(data, KTXCache::Metadata(hash, length));
            if (!networkTexture->_file) {
                qCWarning(modelnetworking) << _url << "file cache failed";
            } else {
                texture->setKtxBacking(networkTexture->_file->getFilepath());
            }
        } else {
            qCWarning(modelnetworking) << "Unable to serialize texture to KTX " << _url;
        }

        // We replace the texture with the one stored in the cache.  This deals with the possible race condition of two different 
        // images with the same hash being loaded concurrently.  Only one of them will make it into the cache by hash first and will
        // be the winner
        texture = textureCache->cacheTextureByHash(hash, texture);
    }

    QMetaObject::invokeMethod(resource.data(), "setImage",
                                Q_ARG(gpu::TexturePointer, texture),
                                Q_ARG(int, texture->getWidth()),
                                Q_ARG(int, texture->getHeight()));
}
