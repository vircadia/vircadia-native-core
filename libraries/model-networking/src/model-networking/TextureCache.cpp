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

#include <QNetworkReply>
#include <QPainter>
#include <QRunnable>
#include <QThreadPool>
#include <QImageReader>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include <gpu/Batch.h>

#include <NumericalConstants.h>
#include <shared/NsightHelpers.h>

#include <Finally.h>
#include <PathUtils.h>

#include "ModelNetworkingLogging.h"

TextureCache::TextureCache() {
    setUnusedResourceCacheSize(0);
    setObjectName("TextureCache");

    // Expose enum Type to JS/QML via properties
    // Despite being one-off, this should be fine, because TextureCache is a SINGLETON_DEPENDENCY
    QObject* type = new QObject(this);
    type->setObjectName("TextureType");
    setProperty("Type", QVariant::fromValue(type));
    auto metaEnum = QMetaEnum::fromType<Type>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        type->setProperty(metaEnum.key(i), metaEnum.value(i));
    }
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

        _permutationNormalTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB), 256, 2));
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
        _whiteTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, 1, 1));
        _whiteTexture->assignStoredMip(0, _whiteTexture->getTexelFormat(), sizeof(OPAQUE_WHITE), OPAQUE_WHITE);
    }
    return _whiteTexture;
}

const gpu::TexturePointer& TextureCache::getGrayTexture() {
    if (!_grayTexture) {
        _grayTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, 1, 1));
        _grayTexture->assignStoredMip(0, _whiteTexture->getTexelFormat(), sizeof(OPAQUE_WHITE), OPAQUE_GRAY);
    }
    return _grayTexture;
}

const gpu::TexturePointer& TextureCache::getBlueTexture() {
    if (!_blueTexture) {
        _blueTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, 1, 1));
        _blueTexture->assignStoredMip(0, _blueTexture->getTexelFormat(), sizeof(OPAQUE_BLUE), OPAQUE_BLUE);
    }
    return _blueTexture;
}

const gpu::TexturePointer& TextureCache::getBlackTexture() {
    if (!_blackTexture) {
        _blackTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, 1, 1));
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
    NetworkTexture::Type type;
    const QByteArray& content;
};

ScriptableResource* TextureCache::prefetch(const QUrl& url, int type) {
    auto byteArray = QByteArray();
    TextureExtra extra = { (Type)type, byteArray };
    return ResourceCache::prefetch(url, &extra);
}

NetworkTexturePointer TextureCache::getTexture(const QUrl& url, Type type, const QByteArray& content) {
    TextureExtra extra = { type, content };
    return ResourceCache::getResource(url, QUrl(), &extra).staticCast<NetworkTexture>();
}


NetworkTexture::TextureLoaderFunc getTextureLoaderForType(NetworkTexture::Type type,
                                                          const QVariantMap& options = QVariantMap()) {
    using Type = NetworkTexture;

    switch (type) {
        case Type::ALBEDO_TEXTURE: {
            return model::TextureUsage::createAlbedoTextureFromImage;
            break;
        }
        case Type::EMISSIVE_TEXTURE: {
            return model::TextureUsage::createEmissiveTextureFromImage;
            break;
        }
        case Type::LIGHTMAP_TEXTURE: {
            return model::TextureUsage::createLightmapTextureFromImage;
            break;
        }
        case Type::CUBE_TEXTURE: {
            if (options.value("generateIrradiance", true).toBool()) {
                return model::TextureUsage::createCubeTextureFromImage;
            } else {
                return model::TextureUsage::createCubeTextureFromImageWithoutIrradiance;
            }
            break;
        }
        case Type::BUMP_TEXTURE: {
            return model::TextureUsage::createNormalTextureFromBumpImage;
            break;
        }
        case Type::NORMAL_TEXTURE: {
            return model::TextureUsage::createNormalTextureFromNormalImage;
            break;
        }
        case Type::ROUGHNESS_TEXTURE: {
            return model::TextureUsage::createRoughnessTextureFromImage;
            break;
        }
        case Type::GLOSS_TEXTURE: {
            return model::TextureUsage::createRoughnessTextureFromGlossImage;
            break;
        }
        case Type::SPECULAR_TEXTURE: {
            return model::TextureUsage::createMetallicTextureFromImage;
            break;
        }
        case Type::CUSTOM_TEXTURE: {
            Q_ASSERT(false);
            return NetworkTexture::TextureLoaderFunc();
            break;
        }
        case Type::DEFAULT_TEXTURE:
        default: {
            return model::TextureUsage::create2DTextureFromImage;
            break;
        }
    }
}

/// Returns a texture version of an image file
gpu::TexturePointer TextureCache::getImageTexture(const QString& path, Type type, QVariantMap options) {
    QImage image = QImage(path);
    auto loader = getTextureLoaderForType(type, options);
    return gpu::TexturePointer(loader(image, QUrl::fromLocalFile(path).fileName().toStdString()));
}

QSharedPointer<Resource> TextureCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
    const void* extra) {
    const TextureExtra* textureExtra = static_cast<const TextureExtra*>(extra);
    auto type = textureExtra ? textureExtra->type : Type::DEFAULT_TEXTURE;
    auto content = textureExtra ? textureExtra->content : QByteArray();
    return QSharedPointer<Resource>(new NetworkTexture(url, type, content),
        &Resource::deleter);
}

NetworkTexture::NetworkTexture(const QUrl& url, Type type, const QByteArray& content) :
    Resource(url),
    _type(type)
{
    _textureSource = std::make_shared<gpu::TextureSource>();

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
    NetworkTexture(url, CUSTOM_TEXTURE, content)
{
    _textureLoader = textureLoader;
}

NetworkTexture::TextureLoaderFunc NetworkTexture::getTextureLoader() const {
    if (_type == CUSTOM_TEXTURE) {
        return _textureLoader;
    }
    return getTextureLoaderForType(_type);
}


class ImageReader : public QRunnable {
public:

    ImageReader(const QWeakPointer<Resource>& resource, const QByteArray& data, const QUrl& url = QUrl());

    virtual void run() override;

private:
    static void listSupportedImageFormats();

    QWeakPointer<Resource> _resource;
    QUrl _url;
    QByteArray _content;
};

void NetworkTexture::downloadFinished(const QByteArray& data) {
    // send the reader off to the thread pool
    QThreadPool::globalInstance()->start(new ImageReader(_self, data, _url));
}

void NetworkTexture::loadContent(const QByteArray& content) {
    QThreadPool::globalInstance()->start(new ImageReader(_self, content, _url));
}

ImageReader::ImageReader(const QWeakPointer<Resource>& resource, const QByteArray& data,
        const QUrl& url) :
    _resource(resource),
    _url(url),
    _content(data)
{
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
    PROFILE_RANGE_EX(__FUNCTION__, 0xffff0000, nullptr);
    auto originalPriority = QThread::currentThread()->priority();
    if (originalPriority == QThread::InheritPriority) {
        originalPriority = QThread::NormalPriority;
    }
    QThread::currentThread()->setPriority(QThread::LowPriority);
    Finally restorePriority([originalPriority]{
        QThread::currentThread()->setPriority(originalPriority);
    });

    if (!_resource.data()) {
        qCWarning(modelnetworking) << "Abandoning load of" << _url << "; could not get strong ref";
        return;
    }

    listSupportedImageFormats();

    // Help the QImage loader by extracting the image file format from the url filename ext.
    // Some tga are not created properly without it.
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

    gpu::TexturePointer texture = nullptr;
    {
        // Double-check the resource still exists between long operations.
        auto resource = _resource.toStrongRef();
        if (!resource) {
            qCWarning(modelnetworking) << "Abandoning load of" << _url << "; could not get strong ref";
            return;
        }

        auto url = _url.toString().toStdString();

        PROFILE_RANGE_EX(__FUNCTION__"::textureLoader", 0xffffff00, nullptr);
        texture.reset(resource.dynamicCast<NetworkTexture>()->getTextureLoader()(image, url));
    }

    // Ensure the resource has not been deleted
    auto resource = _resource.toStrongRef();
    if (!resource) {
        qCWarning(modelnetworking) << "Abandoning load of" << _url << "; could not get strong ref";
    } else {
        QMetaObject::invokeMethod(resource.data(), "setImage",
            Q_ARG(gpu::TexturePointer, texture),
            Q_ARG(int, originalWidth), Q_ARG(int, originalHeight));
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
