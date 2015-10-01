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

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include <QNetworkReply>
#include <QPainter>
#include <QRunnable>
#include <QThreadPool>
#include <qimagereader.h>
#include <PathUtils.h>

#include <gpu/Batch.h>

#include "ModelNetworkingLogging.h"

TextureCache::TextureCache() {
    const qint64 TEXTURE_DEFAULT_UNUSED_MAX_SIZE = DEFAULT_UNUSED_MAX_SIZE;
    setUnusedResourceCacheSize(TEXTURE_DEFAULT_UNUSED_MAX_SIZE);
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
#else
        for (int i = 0; i < 256 * 3; i++) {
            data[i] = rand() % 256;
#endif
        }

        for (int i = 256 * 3; i < 256 * 3 * 2; i += 3) {
            glm::vec3 randvec = glm::sphericalRand(1.0f);
            data[i] = ((randvec.x + 1.0f) / 2.0f) * 255.0f;
            data[i + 1] = ((randvec.y + 1.0f) / 2.0f) * 255.0f;
            data[i + 2] = ((randvec.z + 1.0f) / 2.0f) * 255.0f;
        }

        _permutationNormalTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB), 256, 2));
        _permutationNormalTexture->assignStoredMip(0, _blueTexture->getTexelFormat(), sizeof(data), data);
    }
    return _permutationNormalTexture;
}

const unsigned char OPAQUE_WHITE[] = { 0xFF, 0xFF, 0xFF, 0xFF };
const unsigned char OPAQUE_GRAY[] = { 0x80, 0x80, 0x80, 0xFF };
const unsigned char OPAQUE_BLUE[] = { 0x80, 0x80, 0xFF, 0xFF };
const unsigned char OPAQUE_BLACK[] = { 0x00, 0x00, 0x00, 0xFF };

const gpu::TexturePointer& TextureCache::getWhiteTexture() {
    if (!_whiteTexture) {
        _whiteTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA), 1, 1));
        _whiteTexture->assignStoredMip(0, _whiteTexture->getTexelFormat(), sizeof(OPAQUE_WHITE), OPAQUE_WHITE);
    }
    return _whiteTexture;
}

const gpu::TexturePointer& TextureCache::getGrayTexture() {
    if (!_grayTexture) {
        _grayTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA), 1, 1));
        _grayTexture->assignStoredMip(0, _whiteTexture->getTexelFormat(), sizeof(OPAQUE_WHITE), OPAQUE_GRAY);
    }
    return _grayTexture;
}

const gpu::TexturePointer& TextureCache::getBlueTexture() {
    if (!_blueTexture) {
        _blueTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA), 1, 1));
        _blueTexture->assignStoredMip(0, _blueTexture->getTexelFormat(), sizeof(OPAQUE_BLUE), OPAQUE_BLUE);
    }
    return _blueTexture;
}

const gpu::TexturePointer& TextureCache::getBlackTexture() {
    if (!_blackTexture) {
        _blackTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA), 1, 1));
        _blackTexture->assignStoredMip(0, _whiteTexture->getTexelFormat(), sizeof(OPAQUE_BLACK), OPAQUE_BLACK);
    }
    return _blackTexture;
}


const gpu::TexturePointer& TextureCache::getNormalFittingTexture() {
    if (!_normalFittingTexture) {
        _normalFittingTexture = getImageTexture(PathUtils::resourcesPath() + "images/normalFittingScale.dds");
    }
    return _normalFittingTexture;
}

/// Extra data for creating textures.
class TextureExtra {
public:
    TextureType type;
    const QByteArray& content;
};

NetworkTexturePointer TextureCache::getTexture(const QUrl& url, TextureType type, const QByteArray& content) {
    TextureExtra extra = { type, content };
    return ResourceCache::getResource(url, QUrl(), false, &extra).staticCast<NetworkTexture>();
}

/// Returns a texture version of an image file
gpu::TexturePointer TextureCache::getImageTexture(const QString& path) {
    QImage image = QImage(path).mirrored(false, true);
    gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB);
    gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB);
    if (image.hasAlphaChannel()) {
        formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA);
        formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::BGRA);
    }
    gpu::TexturePointer texture = gpu::TexturePointer(
        gpu::Texture::create2D(formatGPU, image.width(), image.height(), 
            gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
    texture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
    texture->autoGenerateMips(-1);
    return texture;
}


QSharedPointer<Resource> TextureCache::createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra) {
    const TextureExtra* textureExtra = static_cast<const TextureExtra*>(extra);
    return QSharedPointer<Resource>(new NetworkTexture(url, textureExtra->type, textureExtra->content),
        &Resource::allReferencesCleared);
}

Texture::Texture() {
}

Texture::~Texture() {
}

NetworkTexture::NetworkTexture(const QUrl& url, TextureType type, const QByteArray& content) :
    Resource(url, !content.isEmpty()),
    _type(type),
    _width(0),
    _height(0) {
    
    _textureSource.reset(new gpu::TextureSource());

    if (!url.isValid()) {
        _loaded = true;
    }

    std::string theName = url.toString().toStdString();
    // if we have content, load it after we have our self pointer
    if (!content.isEmpty()) {
        _startedLoading = true;
        QMetaObject::invokeMethod(this, "loadContent", Qt::QueuedConnection, Q_ARG(const QByteArray&, content));
    }
}

NetworkTexture::NetworkTexture(const QUrl& url, const TextureLoaderFunc& textureLoader, const QByteArray& content) :
    Resource(url, !content.isEmpty()),
    _type(CUSTOM_TEXTURE),
    _textureLoader(textureLoader),
    _width(0),
    _height(0) {
        
    _textureSource.reset(new gpu::TextureSource());
        
    if (!url.isValid()) {
       _loaded = true;
    }
        
    std::string theName = url.toString().toStdString();
    // if we have content, load it after we have our self pointer
    if (!content.isEmpty()) {
        _startedLoading = true;
        QMetaObject::invokeMethod(this, "loadContent", Qt::QueuedConnection, Q_ARG(const QByteArray&, content));
    }
}
    
NetworkTexture::TextureLoaderFunc NetworkTexture::getTextureLoader() const {
    switch (_type) {
        case CUBE_TEXTURE: {
            return TextureLoaderFunc(model::TextureUsage::createCubeTextureFromImage);
            break;
        }
        case BUMP_TEXTURE: {
            return TextureLoaderFunc(model::TextureUsage::createNormalTextureFromBumpImage);
            break;
        }
        case NORMAL_TEXTURE: {
            return TextureLoaderFunc(model::TextureUsage::createNormalTextureFromNormalImage);
            break;
        }
        case CUSTOM_TEXTURE: {
            return _textureLoader;
            break;
        }
        case DEFAULT_TEXTURE:
        case SPECULAR_TEXTURE:
        case EMISSIVE_TEXTURE:
        default: {
            return TextureLoaderFunc(model::TextureUsage::create2DTextureFromImage);
            break;
        }
    }
}
    

class ImageReader : public QRunnable {
public:

    ImageReader(const QWeakPointer<Resource>& texture, const NetworkTexture::TextureLoaderFunc& textureLoader, const QByteArray& data, const QUrl& url = QUrl());
    
    virtual void run();

private:
    
    QWeakPointer<Resource> _texture;
    NetworkTexture::TextureLoaderFunc _textureLoader;
    QUrl _url;
    QByteArray _content;
};

void NetworkTexture::downloadFinished(const QByteArray& data) {
    // send the reader off to the thread pool
    QThreadPool::globalInstance()->start(new ImageReader(_self, getTextureLoader(), data, _url));
}

void NetworkTexture::loadContent(const QByteArray& content) {
    QThreadPool::globalInstance()->start(new ImageReader(_self, getTextureLoader(), content, _url));
}

ImageReader::ImageReader(const QWeakPointer<Resource>& texture, const NetworkTexture::TextureLoaderFunc& textureLoader, const QByteArray& data,
        const QUrl& url) :
    _texture(texture),
    _textureLoader(textureLoader),
    _url(url),
    _content(data)
{
    
}

std::once_flag onceListSupportedFormatsflag;
void listSupportedImageFormats() {
    std::call_once(onceListSupportedFormatsflag, [](){
        auto supportedFormats = QImageReader::supportedImageFormats();
        QString formats;
        foreach(const QByteArray& f, supportedFormats) {
            formats += QString(f) + ",";
        }
        qCDebug(modelnetworking) << "List of supported Image formats:" << formats;
    });
}

void ImageReader::run() {
    QSharedPointer<Resource> texture = _texture.toStrongRef();
    if (texture.isNull()) {
        return;
    }

    listSupportedImageFormats();

    // try to help the QImage loader by extracting the image file format from the url filename ext
    // Some tga are not created properly for example without it
    auto filename = _url.fileName().toStdString();
    auto filenameExtension = filename.substr(filename.find_last_of('.') + 1);
    QImage image = QImage::fromData(_content, filenameExtension.c_str());

    // Note that QImage.format is the pixel format which is different from the "format" of the image file...
    auto imageFormat = image.format(); 
    int originalWidth = image.width();
    int originalHeight = image.height();
    
    if (originalWidth == 0 || originalHeight == 0 || imageFormat == QImage::Format_Invalid) {
        if (filenameExtension.empty()) {
            qCDebug(modelnetworking) << "QImage failed to create from content, no file extension:" << _url;
        } else {
            qCDebug(modelnetworking) << "QImage failed to create from content" << _url;
        }
        return;
    }

    gpu::Texture* theTexture = nullptr;
    auto ntex = dynamic_cast<NetworkTexture*>(&*texture);
    if (ntex) {
        theTexture = ntex->getTextureLoader()(image, _url.toString().toStdString());
    }

    QMetaObject::invokeMethod(texture.data(), "setImage", 
        Q_ARG(const QImage&, image),
        Q_ARG(void*, theTexture),
        Q_ARG(int, originalWidth), Q_ARG(int, originalHeight));


}

void NetworkTexture::setImage(const QImage& image, void* voidTexture, int originalWidth,
                              int originalHeight) {
    _originalWidth = originalWidth;
    _originalHeight = originalHeight;
    
    gpu::Texture* texture = static_cast<gpu::Texture*>(voidTexture);
    
    // Passing ownership
    _textureSource->resetTexture(texture);
    auto gpuTexture = _textureSource->getGPUTexture();

    if (gpuTexture) {
        _width = gpuTexture->getWidth();
        _height = gpuTexture->getHeight();
    } else {
        _width = _height = 0;
    }
    
    finishedLoading(true);

    imageLoaded(image);
}

void NetworkTexture::imageLoaded(const QImage& image) {
    // nothing by default
}

