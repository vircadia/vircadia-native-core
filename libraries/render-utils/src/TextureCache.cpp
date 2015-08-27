//
//  TextureCache.cpp
//  interface/src/renderer
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
#include "PathUtils.h"

#include <gpu/Batch.h>



#include "RenderUtilsLogging.h"

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

NetworkTexturePointer TextureCache::getTexture(const QUrl& url, TextureType type, bool dilatable, const QByteArray& content) {
    if (!dilatable) {
        TextureExtra extra = { type, content };
        return ResourceCache::getResource(url, QUrl(), false, &extra).staticCast<NetworkTexture>();
    }
    NetworkTexturePointer texture = _dilatableNetworkTextures.value(url);
    if (texture.isNull()) {
        texture = NetworkTexturePointer(new DilatableNetworkTexture(url, content), &Resource::allReferencesCleared);
        texture->setSelf(texture);
        texture->setCache(this);
        _dilatableNetworkTextures.insert(url, texture);
    } else {
        removeUnusedResource(texture);
    }
    return texture;
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
    _translucent(false),
    _width(0),
    _height(0) {
    
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

class ImageReader : public QRunnable {
public:

    ImageReader(const QWeakPointer<Resource>& texture, TextureType type, const QByteArray& data, const QUrl& url = QUrl());
    
    virtual void run();

private:
    
    QWeakPointer<Resource> _texture;
    TextureType _type;
    QUrl _url;
    QByteArray _content;
};

void NetworkTexture::downloadFinished(const QByteArray& data) {
    // send the reader off to the thread pool
    QThreadPool::globalInstance()->start(new ImageReader(_self, _type, data, _url));
}

void NetworkTexture::loadContent(const QByteArray& content) {
    QThreadPool::globalInstance()->start(new ImageReader(_self, _type, content, _url));
}

ImageReader::ImageReader(const QWeakPointer<Resource>& texture, TextureType type, const QByteArray& data,
        const QUrl& url) :
    _texture(texture),
    _type(type),
    _url(url),
    _content(data) {
}

std::once_flag onceListSupportedFormatsflag;
void listSupportedImageFormats() {
    std::call_once(onceListSupportedFormatsflag, [](){
        auto supportedFormats = QImageReader::supportedImageFormats();
        QString formats;
        foreach(const QByteArray& f, supportedFormats) {
            formats += QString(f) + ",";
        }
        qCDebug(renderutils) << "List of supported Image formats:" << formats;
    });
}


class CubeLayout {
public:
    int _widthRatio = 1;
    int _heightRatio = 1;
 
    class Face {
    public:
        int _x = 0;
        int _y = 0;
        bool _horizontalMirror = false;
        bool _verticalMirror = false;

        Face() {}
        Face(int x, int y, bool horizontalMirror, bool verticalMirror) : _x(x), _y(y), _horizontalMirror(horizontalMirror), _verticalMirror(verticalMirror) {}
    };
                
    Face _faceXPos;
    Face _faceXNeg;
    Face _faceYPos;
    Face _faceYNeg;
    Face _faceZPos;
    Face _faceZNeg;
 
    CubeLayout(int wr, int hr, Face fXP, Face fXN, Face fYP, Face fYN, Face fZP, Face fZN) :
            _widthRatio(wr),
            _heightRatio(hr),
            _faceXPos(fXP),
            _faceXNeg(fXN),
            _faceYPos(fYP),
            _faceYNeg(fYN),
            _faceZPos(fZP),
            _faceZNeg(fZN) {}
};

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
            qCDebug(renderutils) << "QImage failed to create from content, no file extension:" << _url;
        } else {
            qCDebug(renderutils) << "QImage failed to create from content" << _url;
        }
        return;
    }

    int imageArea = image.width() * image.height();
    auto ntex = dynamic_cast<NetworkTexture*>(&*texture);
    if (ntex && (ntex->getType() == CUBE_TEXTURE)) {
        qCDebug(renderutils) << "Cube map size:" << _url << image.width() << image.height();
    }
    
    int opaquePixels = 0;
    int translucentPixels = 0;
    bool isTransparent = false;
    int redTotal = 0, greenTotal = 0, blueTotal = 0, alphaTotal = 0;
    const int EIGHT_BIT_MAXIMUM = 255;
        QColor averageColor(EIGHT_BIT_MAXIMUM, EIGHT_BIT_MAXIMUM, EIGHT_BIT_MAXIMUM);

    if (!image.hasAlphaChannel()) {
        if (image.format() != QImage::Format_RGB888) {
            image = image.convertToFormat(QImage::Format_RGB888);
        }
       // int redTotal = 0, greenTotal = 0, blueTotal = 0;
        for (int y = 0; y < image.height(); y++) {
            for (int x = 0; x < image.width(); x++) {
                QRgb rgb = image.pixel(x, y);
                redTotal += qRed(rgb);
                greenTotal += qGreen(rgb);
                blueTotal += qBlue(rgb);
            }
        }
        if (imageArea > 0) {
            averageColor.setRgb(redTotal / imageArea, greenTotal / imageArea, blueTotal / imageArea);
        }
    } else {
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }
    
        // check for translucency/false transparency
       // int opaquePixels = 0;
       // int translucentPixels = 0;
       // int redTotal = 0, greenTotal = 0, blueTotal = 0, alphaTotal = 0;
        for (int y = 0; y < image.height(); y++) {
            for (int x = 0; x < image.width(); x++) {
                QRgb rgb = image.pixel(x, y);
                redTotal += qRed(rgb);
                greenTotal += qGreen(rgb);
                blueTotal += qBlue(rgb);
                int alpha = qAlpha(rgb);
                alphaTotal += alpha;
                if (alpha == EIGHT_BIT_MAXIMUM) {
                    opaquePixels++;
                } else if (alpha != 0) {
                    translucentPixels++;
                }
            }
        }
        if (opaquePixels == imageArea) {
            qCDebug(renderutils) << "Image with alpha channel is completely opaque:" << _url;
            image = image.convertToFormat(QImage::Format_RGB888);
        }

        averageColor = QColor(redTotal / imageArea,
            greenTotal / imageArea, blueTotal / imageArea, alphaTotal / imageArea);

        isTransparent = (translucentPixels >= imageArea / 2);
    }

    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {

       // bool isLinearRGB = true; //(_type == NORMAL_TEXTURE) || (_type == EMISSIVE_TEXTURE);
        bool isLinearRGB = !(_type == CUBE_TEXTURE); //(_type == NORMAL_TEXTURE) || (_type == EMISSIVE_TEXTURE);

        gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::UINT8, (isLinearRGB ? gpu::RGB : gpu::SRGB));
        gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::UINT8, (isLinearRGB ? gpu::RGB : gpu::SRGB));
        if (image.hasAlphaChannel()) {
            formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, (isLinearRGB ? gpu::RGBA : gpu::SRGBA));
            formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, (isLinearRGB ? gpu::BGRA : gpu::SBGRA));
        }
        
        if (_type == CUBE_TEXTURE) {

            const CubeLayout CUBEMAP_LAYOUTS[] = {
                // Here is the expected layout for the faces in an image with the 1/6 aspect ratio:
                //
                //         WIDTH
                //       <------>
                //    ^  +------+
                //    |  |      |
                //    |  |  +X  |
                //    |  |      |
                //    H  +------+
                //    E  |      |
                //    I  |  -X  |
                //    G  |      |
                //    H  +------+
                //    T  |      |
                //    |  |  +Y  |
                //    |  |      |
                //    |  +------+
                //    |  |      |
                //    |  |  -Y  |
                //    |  |      |
                //    H  +------+
                //    E  |      |
                //    I  |  +Z  |
                //    G  |      |
                //    H  +------+
                //    T  |      |
                //    |  |  -Z  |
                //    |  |      |
                //    V  +------+
                // 
                //    FaceWidth = width = height / 6
                {   1, 6,
                    {0, 0, true, false},
                    {0, 1, true, false},
                    {0, 2, false, true},
                    {0, 3, false, true},
                    {0, 4, true, false},
                    {0, 5, true, false}
                },

                // Here is the expected layout for the faces in an image with the 3/4 aspect ratio:
                //
                //       <-----------WIDTH----------->
                //    ^  +------+------+------+------+
                //    |  |      |      |      |      |
                //    |  |      |  +Y  |      |      |
                //    |  |      |      |      |      |
                //    H  +------+------+------+------+
                //    E  |      |      |      |      |
                //    I  |  -X  |  -Z  |  +X  |  +Z  |
                //    G  |      |      |      |      |
                //    H  +------+------+------+------+
                //    T  |      |      |      |      |
                //    |  |      |  -Y  |      |      |
                //    |  |      |      |      |      |
                //    V  +------+------+------+------+
                // 
                //    FaceWidth = width / 4 = height / 3
                {   4, 3,
                    {2, 1, true, false},
                    {0, 1, true, false},
                    {1, 0, false, true},
                    {1, 2, false, true},
                    {3, 1, true, false},
                    {1, 1, true, false}
                },

                // Here is the expected layout for the faces in an image with the 4/3 aspect ratio:
                //
                //       <-------WIDTH-------->
                //    ^  +------+------+------+
                //    |  |      |      |      |
                //    |  |      |  +Y  |      |
                //    |  |      |      |      |
                //    H  +------+------+------+
                //    E  |      |      |      |
                //    I  |  -X  |  -Z  |  +X  |
                //    G  |      |      |      |
                //    H  +------+------+------+
                //    T  |      |      |      |
                //    |  |      |  -Y  |      |
                //    |  |      |      |      |
                //    |  +------+------+------+
                //    |  |      |      |      |
                //    |  |      |  +Z! |      | <+Z is upside down!
                //    |  |      |      |      |
                //    V  +------+------+------+
                // 
                //    FaceWidth = width / 3 = height / 4
                {   3, 4,
                    {2, 1, true, false},
                    {0, 1, true, false},
                    {1, 0, false, true},
                    {1, 2, false, true},
                    {1, 3, false, true},
                    {1, 1, true, false}
                }
            };
            const int NUM_CUBEMAP_LAYOUTS = sizeof(CUBEMAP_LAYOUTS) / sizeof(CubeLayout);

            // Find the layout of the cubemap in the 2D image
            int foundLayout = -1;
            for (int i = 0; i < NUM_CUBEMAP_LAYOUTS; i++) {
                if ((image.height() * CUBEMAP_LAYOUTS[i]._widthRatio) == (image.width() * CUBEMAP_LAYOUTS[i]._heightRatio)) {
                    foundLayout = i;
                    break;
                }
            }

            std::vector<QImage> faces;
            // If found, go extract the faces as separate images
            if (foundLayout >= 0) {
                auto& layout = CUBEMAP_LAYOUTS[foundLayout];
                int faceWidth = image.width() / layout._widthRatio;

                faces.push_back(image.copy(QRect(layout._faceXPos._x * faceWidth, layout._faceXPos._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceXPos._horizontalMirror, layout._faceXPos._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceXNeg._x * faceWidth, layout._faceXNeg._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceXNeg._horizontalMirror, layout._faceXNeg._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceYPos._x * faceWidth, layout._faceYPos._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceYPos._horizontalMirror, layout._faceYPos._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceYNeg._x * faceWidth, layout._faceYNeg._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceYNeg._horizontalMirror, layout._faceYNeg._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceZPos._x * faceWidth, layout._faceZPos._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceZPos._horizontalMirror, layout._faceZPos._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceZNeg._x * faceWidth, layout._faceZNeg._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceZNeg._horizontalMirror, layout._faceZNeg._verticalMirror));
            } else {
                qCDebug(renderutils) << "Failed to find a known cube map layout from this image:" << _url;
                return;
            }

            // If the 6 faces have been created go on and define the true Texture
            if (faces.size() == gpu::Texture::NUM_FACES_PER_TYPE[gpu::Texture::TEX_CUBE]) {
                theTexture = gpu::Texture::createCube(formatGPU, faces[0].width(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR, gpu::Sampler::WRAP_CLAMP));
                theTexture->autoGenerateMips(-1);
                int f = 0;
                for (auto& face : faces) {
                    theTexture->assignStoredMipFace(0, formatMip, face.byteCount(), face.constBits(), f);
                    f++;
                }
                
                // GEnerate irradiance while we are at it
                theTexture->generateIrradiance();
            }

        } else {
            theTexture = (gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
            theTexture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
            theTexture->autoGenerateMips(-1);
        }
    }

    QMetaObject::invokeMethod(texture.data(), "setImage", 
        Q_ARG(const QImage&, image),
        Q_ARG(void*, theTexture),
        Q_ARG(bool, isTransparent),
        Q_ARG(const QColor&, averageColor),
        Q_ARG(int, originalWidth), Q_ARG(int, originalHeight));


}

void NetworkTexture::setImage(const QImage& image, void* voidTexture, bool translucent, const QColor& averageColor, int originalWidth,
                              int originalHeight) {
    _translucent = translucent;
    _averageColor = averageColor;
    _originalWidth = originalWidth;
    _originalHeight = originalHeight;
    
    gpu::Texture* texture = static_cast<gpu::Texture*>(voidTexture);
    // Passing ownership
    _gpuTexture.reset(texture);

    if (_gpuTexture) {
        _width = _gpuTexture->getWidth();
        _height = _gpuTexture->getHeight(); 
    } else {
        _width = _height = 0;
    }
    
    finishedLoading(true);

    imageLoaded(image);
}

void NetworkTexture::imageLoaded(const QImage& image) {
    // nothing by default
}

DilatableNetworkTexture::DilatableNetworkTexture(const QUrl& url, const QByteArray& content) :
    NetworkTexture(url, DEFAULT_TEXTURE, content),
    _innerRadius(0),
    _outerRadius(0)
{
}

QSharedPointer<Texture> DilatableNetworkTexture::getDilatedTexture(float dilation) {
    QSharedPointer<Texture> texture = _dilatedTextures.value(dilation);
    if (texture.isNull()) {
        texture = QSharedPointer<Texture>::create();
        
        if (!_image.isNull()) {
            QImage dilatedImage = _image;
            QPainter painter;
            painter.begin(&dilatedImage);
            QPainterPath path;
            qreal radius = glm::mix((float) _innerRadius, (float) _outerRadius, dilation);
            path.addEllipse(QPointF(_image.width() / 2.0, _image.height() / 2.0), radius, radius);
            painter.fillPath(path, Qt::black);
            painter.end();

            bool isLinearRGB = true;// (_type == NORMAL_TEXTURE) || (_type == EMISSIVE_TEXTURE);
            gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::UINT8, (isLinearRGB ? gpu::RGB : gpu::SRGB));
            gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::UINT8, (isLinearRGB ? gpu::RGB : gpu::SRGB));
            if (dilatedImage.hasAlphaChannel()) {
                formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, (isLinearRGB ? gpu::RGBA : gpu::SRGBA));
                // FIXME either remove the ?: operator or provide different arguments depending on linear
                formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, (isLinearRGB ? gpu::BGRA : gpu::BGRA));
            }
            texture->_gpuTexture = gpu::TexturePointer(gpu::Texture::create2D(formatGPU, dilatedImage.width(), dilatedImage.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
            texture->_gpuTexture->assignStoredMip(0, formatMip, dilatedImage.byteCount(), dilatedImage.constBits());
            texture->_gpuTexture->autoGenerateMips(-1);

        }
        
        _dilatedTextures.insert(dilation, texture);
    }
    return texture;
}

void DilatableNetworkTexture::imageLoaded(const QImage& image) {
    _image = image;
    
    // scan out from the center to find inner and outer radii
    int halfWidth = image.width() / 2;
    int halfHeight = image.height() / 2;
    const int BLACK_THRESHOLD = 32;
    while (_innerRadius < halfWidth && qGray(image.pixel(halfWidth + _innerRadius, halfHeight)) < BLACK_THRESHOLD) {
        _innerRadius++;
    }
    _outerRadius = _innerRadius;
    const int TRANSPARENT_THRESHOLD = 32;
    while (_outerRadius < halfWidth && qAlpha(image.pixel(halfWidth + _outerRadius, halfHeight)) > TRANSPARENT_THRESHOLD) {
        _outerRadius++;
    }
    
    // clear out any textures we generated before loading
    _dilatedTextures.clear();
}

void DilatableNetworkTexture::reinsert() {
    static_cast<TextureCache*>(_cache.data())->_dilatableNetworkTextures.insert(_url,
        qWeakPointerCast<NetworkTexture, Resource>(_self));    
}

