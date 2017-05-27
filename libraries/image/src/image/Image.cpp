//
//  Image.cpp
//  image/src/image
//
//  Created by Clement Brisset on 4/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Image.h"

#include <nvtt/nvtt.h>

#include <QUrl>
#include <QImage>
#include <QBuffer>
#include <QImageReader>

#include <Finally.h>
#include <Profile.h>
#include <StatTracker.h>
#include <GLMHelpers.h>
#include <SettingHandle.h>

#include "ImageLogging.h"

using namespace gpu;

#define CPU_MIPMAPS 1

static std::mutex settingsMutex;
static Setting::Handle<bool> compressColorTextures("hifi.graphics.compressColorTextures", false);
static Setting::Handle<bool> compressNormalTextures("hifi.graphics.compressNormalTextures", false);
static Setting::Handle<bool> compressGrayscaleTextures("hifi.graphics.compressGrayscaleTextures", false);
static Setting::Handle<bool> compressCubeTextures("hifi.graphics.compressCubeTextures", false);

static const glm::uvec2 SPARSE_PAGE_SIZE(128);
static const glm::uvec2 MAX_TEXTURE_SIZE(4096);
bool DEV_DECIMATE_TEXTURES = false;
std::atomic<size_t> DECIMATED_TEXTURE_COUNT{ 0 };
std::atomic<size_t> RECTIFIED_TEXTURE_COUNT{ 0 };

bool needsSparseRectification(const glm::uvec2& size) {
    // Don't attempt to rectify small textures (textures less than the sparse page size in any dimension)
    if (glm::any(glm::lessThan(size, SPARSE_PAGE_SIZE))) {
        return false;
    }

    // Don't rectify textures that are already an exact multiple of sparse page size
    if (glm::uvec2(0) == (size % SPARSE_PAGE_SIZE)) {
        return false;
    }

    // Texture is not sparse compatible, but is bigger than the sparse page size in both dimensions, rectify!
    return true;
}

glm::uvec2 rectifyToSparseSize(const glm::uvec2& size) {
    glm::uvec2 pages = ((size / SPARSE_PAGE_SIZE) + glm::clamp(size % SPARSE_PAGE_SIZE, glm::uvec2(0), glm::uvec2(1)));
    glm::uvec2 result = pages * SPARSE_PAGE_SIZE;
    return result;
}


namespace image {

TextureUsage::TextureLoader TextureUsage::getTextureLoaderForType(Type type, const QVariantMap& options) {
    switch (type) {
        case ALBEDO_TEXTURE:
            return image::TextureUsage::createAlbedoTextureFromImage;
        case EMISSIVE_TEXTURE:
            return image::TextureUsage::createEmissiveTextureFromImage;
        case LIGHTMAP_TEXTURE:
            return image::TextureUsage::createLightmapTextureFromImage;
        case CUBE_TEXTURE:
            if (options.value("generateIrradiance", true).toBool()) {
                return image::TextureUsage::createCubeTextureFromImage;
            } else {
                return image::TextureUsage::createCubeTextureFromImageWithoutIrradiance;
            }
        case BUMP_TEXTURE:
            return image::TextureUsage::createNormalTextureFromBumpImage;
        case NORMAL_TEXTURE:
            return image::TextureUsage::createNormalTextureFromNormalImage;
        case ROUGHNESS_TEXTURE:
            return image::TextureUsage::createRoughnessTextureFromImage;
        case GLOSS_TEXTURE:
            return image::TextureUsage::createRoughnessTextureFromGlossImage;
        case SPECULAR_TEXTURE:
            return image::TextureUsage::createMetallicTextureFromImage;
        case STRICT_TEXTURE:
            return image::TextureUsage::createStrict2DTextureFromImage;

        case DEFAULT_TEXTURE:
        default:
            return image::TextureUsage::create2DTextureFromImage;
    }
}

gpu::TexturePointer TextureUsage::createStrict2DTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, srcImageName, true);
}

gpu::TexturePointer TextureUsage::create2DTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, srcImageName, false);
}

gpu::TexturePointer TextureUsage::createAlbedoTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, srcImageName, false);
}

gpu::TexturePointer TextureUsage::createEmissiveTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, srcImageName, false);
}

gpu::TexturePointer TextureUsage::createLightmapTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, srcImageName, false);
}

gpu::TexturePointer TextureUsage::createNormalTextureFromNormalImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureNormalMapFromImage(srcImage, srcImageName, false);
}

gpu::TexturePointer TextureUsage::createNormalTextureFromBumpImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureNormalMapFromImage(srcImage, srcImageName, true);
}

gpu::TexturePointer TextureUsage::createRoughnessTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureGrayscaleFromImage(srcImage, srcImageName, false);
}

gpu::TexturePointer TextureUsage::createRoughnessTextureFromGlossImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureGrayscaleFromImage(srcImage, srcImageName, true);
}

gpu::TexturePointer TextureUsage::createMetallicTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureGrayscaleFromImage(srcImage, srcImageName, false);
}

gpu::TexturePointer TextureUsage::createCubeTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return processCubeTextureColorFromImage(srcImage, srcImageName, true);
}

gpu::TexturePointer TextureUsage::createCubeTextureFromImageWithoutIrradiance(const QImage& srcImage, const std::string& srcImageName) {
    return processCubeTextureColorFromImage(srcImage, srcImageName, false);
}


bool isColorTexturesCompressionEnabled() {
#if CPU_MIPMAPS
    std::lock_guard<std::mutex> guard(settingsMutex);
    return compressColorTextures.get();
#else
    return false;
#endif
}

bool isNormalTexturesCompressionEnabled() {
#if CPU_MIPMAPS
    std::lock_guard<std::mutex> guard(settingsMutex);
    return compressNormalTextures.get();
#else
    return false;
#endif
}

bool isGrayscaleTexturesCompressionEnabled() {
#if CPU_MIPMAPS
    std::lock_guard<std::mutex> guard(settingsMutex);
    return compressGrayscaleTextures.get();
#else
    return false;
#endif
}

bool isCubeTexturesCompressionEnabled() {
#if CPU_MIPMAPS
    std::lock_guard<std::mutex> guard(settingsMutex);
    return compressCubeTextures.get();
#else
    return false;
#endif
}

void setColorTexturesCompressionEnabled(bool enabled) {
    std::lock_guard<std::mutex> guard(settingsMutex);
    compressColorTextures.set(enabled);
}

void setNormalTexturesCompressionEnabled(bool enabled) {
    std::lock_guard<std::mutex> guard(settingsMutex);
    compressNormalTextures.set(enabled);
}

void setGrayscaleTexturesCompressionEnabled(bool enabled) {
    std::lock_guard<std::mutex> guard(settingsMutex);
    compressGrayscaleTextures.set(enabled);
}

void setCubeTexturesCompressionEnabled(bool enabled) {
    std::lock_guard<std::mutex> guard(settingsMutex);
    compressCubeTextures.set(enabled);
}


gpu::TexturePointer processImage(const QByteArray& content, const std::string& filename, int maxNumPixels, TextureUsage::Type textureType) {
    // Help the QImage loader by extracting the image file format from the url filename ext.
    // Some tga are not created properly without it.
    auto filenameExtension = filename.substr(filename.find_last_of('.') + 1);
    QBuffer buffer;
    buffer.setData(content);
    QImageReader imageReader(&buffer, filenameExtension.c_str());
    QImage image;

    if (imageReader.canRead()) {
        image = imageReader.read();
    } else {
        // Extension could be incorrect, try to detect the format from the content
        QImageReader newImageReader;
        newImageReader.setDecideFormatFromContent(true);
        buffer.setData(content);
        newImageReader.setDevice(&buffer);

        if (newImageReader.canRead()) {
            qCWarning(imagelogging) << "Image file" << filename.c_str() << "has extension" << filenameExtension.c_str()
                                    << "but is actually a" << qPrintable(newImageReader.format()) << "(recovering)";
            image = newImageReader.read();
        }
    }

    int imageWidth = image.width();
    int imageHeight = image.height();

    // Validate that the image loaded
    if (imageWidth == 0 || imageHeight == 0 || image.format() == QImage::Format_Invalid) {
        QString reason(image.format() == QImage::Format_Invalid ? "(Invalid Format)" : "(Size is invalid)");
        qCWarning(imagelogging) << "Failed to load" << filename.c_str() << qPrintable(reason);
        return nullptr;
    }

    // Validate the image is less than _maxNumPixels, and downscale if necessary
    if (imageWidth * imageHeight > maxNumPixels) {
        float scaleFactor = sqrtf(maxNumPixels / (float)(imageWidth * imageHeight));
        int originalWidth = imageWidth;
        int originalHeight = imageHeight;
        imageWidth = (int)(scaleFactor * (float)imageWidth + 0.5f);
        imageHeight = (int)(scaleFactor * (float)imageHeight + 0.5f);
        QImage newImage = image.scaled(QSize(imageWidth, imageHeight), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        image.swap(newImage);
        qCDebug(imagelogging).nospace() << "Downscaled " << filename.c_str() << " (" <<
            QSize(originalWidth, originalHeight) << " to " <<
            QSize(imageWidth, imageHeight) << ")";
    }
    
    auto loader = TextureUsage::getTextureLoaderForType(textureType);
    auto texture = loader(image, filename);

    return texture;
}



QImage processSourceImage(const QImage& srcImage, bool cubemap) {
    PROFILE_RANGE(resource_parse, "processSourceImage");
    const glm::uvec2 srcImageSize = toGlm(srcImage.size());
    glm::uvec2 targetSize = srcImageSize;

    while (glm::any(glm::greaterThan(targetSize, MAX_TEXTURE_SIZE))) {
        targetSize /= 2;
    }
    if (targetSize != srcImageSize) {
        ++DECIMATED_TEXTURE_COUNT;
    }

    if (!cubemap && needsSparseRectification(targetSize)) {
        ++RECTIFIED_TEXTURE_COUNT;
        targetSize = rectifyToSparseSize(targetSize);
    }

    if (DEV_DECIMATE_TEXTURES && glm::all(glm::greaterThanEqual(targetSize / SPARSE_PAGE_SIZE, glm::uvec2(2)))) {
        targetSize /= 2;
    }

    if (targetSize != srcImageSize) {
        PROFILE_RANGE(resource_parse, "processSourceImage Rectify");
        qCDebug(imagelogging) << "Resizing texture from " << srcImageSize.x << "x" << srcImageSize.y << " to " << targetSize.x << "x" << targetSize.y;
        return srcImage.scaled(fromGlm(targetSize), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    return srcImage;
}

struct MyOutputHandler : public nvtt::OutputHandler {
    MyOutputHandler(gpu::Texture* texture, int face) : _texture(texture), _face(face) {}

    virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) override {
        _size = size;
        _miplevel = miplevel;

        _data = static_cast<gpu::Byte*>(malloc(size));
        _current = _data;
    }
    virtual bool writeData(const void* data, int size) override {
        assert(_current + size <= _data + _size);
        memcpy(_current, data, size);
        _current += size;
        return true;
    }
    virtual void endImage() override {
        if (_face >= 0) {
            _texture->assignStoredMipFace(_miplevel, _face, _size, static_cast<const gpu::Byte*>(_data));
        } else {
            _texture->assignStoredMip(_miplevel, _size, static_cast<const gpu::Byte*>(_data));
        }
        free(_data);
        _data = nullptr;
    }

    gpu::Byte* _data{ nullptr };
    gpu::Byte* _current{ nullptr };
    gpu::Texture* _texture{ nullptr };
    int _miplevel = 0;
    int _size = 0;
    int _face = -1;
};
struct MyErrorHandler : public nvtt::ErrorHandler {
    virtual void error(nvtt::Error e) override {
        qCWarning(imagelogging) << "Texture compression error:" << nvtt::errorString(e);
    }
};

void generateMips(gpu::Texture* texture, QImage& image, int face = -1) {
#if CPU_MIPMAPS
    PROFILE_RANGE(resource_parse, "generateMips");

    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    const int width = image.width(), height = image.height();
    const void* data = static_cast<const void*>(image.constBits());

    nvtt::TextureType textureType = nvtt::TextureType_2D;
    nvtt::InputFormat inputFormat = nvtt::InputFormat_BGRA_8UB;
    nvtt::WrapMode wrapMode = nvtt::WrapMode_Mirror;
    nvtt::RoundMode roundMode = nvtt::RoundMode_None;
    nvtt::AlphaMode alphaMode = nvtt::AlphaMode_None;

    float inputGamma = 2.2f;
    float outputGamma = 2.2f;

    nvtt::InputOptions inputOptions;
    inputOptions.setTextureLayout(textureType, width, height);
    inputOptions.setMipmapData(data, width, height);

    inputOptions.setFormat(inputFormat);
    inputOptions.setGamma(inputGamma, outputGamma);
    inputOptions.setAlphaMode(alphaMode);
    inputOptions.setWrapMode(wrapMode);
    inputOptions.setRoundMode(roundMode);

    inputOptions.setMipmapGeneration(true);
    inputOptions.setMipmapFilter(nvtt::MipmapFilter_Box);

    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setQuality(nvtt::Quality_Production);

    auto mipFormat = texture->getStoredMipFormat();
    if (mipFormat == gpu::Element::COLOR_COMPRESSED_SRGB) {
        compressionOptions.setFormat(nvtt::Format_BC1);
    } else if (mipFormat == gpu::Element::COLOR_COMPRESSED_SRGBA_MASK) {
        alphaMode = nvtt::AlphaMode_Transparency;
        compressionOptions.setFormat(nvtt::Format_BC1a);
    } else if (mipFormat == gpu::Element::COLOR_COMPRESSED_SRGBA) {
        alphaMode = nvtt::AlphaMode_Transparency;
        compressionOptions.setFormat(nvtt::Format_BC3);
    } else if (mipFormat == gpu::Element::COLOR_COMPRESSED_RED) {
        compressionOptions.setFormat(nvtt::Format_BC4);
    } else if (mipFormat == gpu::Element::COLOR_COMPRESSED_XY) {
        compressionOptions.setFormat(nvtt::Format_BC5);
    } else if (mipFormat == gpu::Element::COLOR_COMPRESSED_SRGBA_HIGH) {
        alphaMode = nvtt::AlphaMode_Transparency;
        compressionOptions.setFormat(nvtt::Format_BC7);
    } else if (mipFormat == gpu::Element::COLOR_RGBA_32) {
        compressionOptions.setFormat(nvtt::Format_RGBA);
        compressionOptions.setPixelType(nvtt::PixelType_UnsignedNorm);
        compressionOptions.setPitchAlignment(4);
        compressionOptions.setPixelFormat(32,
                                          0x000000FF,
                                          0x0000FF00,
                                          0x00FF0000,
                                          0xFF000000);
        inputGamma = 1.0f;
        outputGamma = 1.0f;
    } else if (mipFormat == gpu::Element::COLOR_BGRA_32) {
        compressionOptions.setFormat(nvtt::Format_RGBA);
        compressionOptions.setPixelType(nvtt::PixelType_UnsignedNorm);
        compressionOptions.setPitchAlignment(4);
        compressionOptions.setPixelFormat(32,
                                          0x00FF0000,
                                          0x0000FF00,
                                          0x000000FF,
                                          0xFF000000);
        inputGamma = 1.0f;
        outputGamma = 1.0f;
    } else if (mipFormat == gpu::Element::COLOR_SRGBA_32) {
        compressionOptions.setFormat(nvtt::Format_RGBA);
        compressionOptions.setPixelType(nvtt::PixelType_UnsignedNorm);
        compressionOptions.setPitchAlignment(4);
        compressionOptions.setPixelFormat(32,
                                          0x000000FF,
                                          0x0000FF00,
                                          0x00FF0000,
                                          0xFF000000);
    } else if (mipFormat == gpu::Element::COLOR_SBGRA_32) {
        compressionOptions.setFormat(nvtt::Format_RGBA);
        compressionOptions.setPixelType(nvtt::PixelType_UnsignedNorm);
        compressionOptions.setPitchAlignment(4);
        compressionOptions.setPixelFormat(32,
                                          0x00FF0000,
                                          0x0000FF00,
                                          0x000000FF,
                                          0xFF000000);
    } else if (mipFormat == gpu::Element::COLOR_R_8) {
        compressionOptions.setFormat(nvtt::Format_RGB);
        compressionOptions.setPixelType(nvtt::PixelType_UnsignedNorm);
        compressionOptions.setPitchAlignment(4);
        compressionOptions.setPixelFormat(8, 0, 0, 0);
    } else if (mipFormat == gpu::Element::VEC2NU8_XY) {
        inputOptions.setNormalMap(true);
        compressionOptions.setFormat(nvtt::Format_RGBA);
        compressionOptions.setPixelType(nvtt::PixelType_UnsignedNorm);
        compressionOptions.setPitchAlignment(4);
        compressionOptions.setPixelFormat(8, 8, 0, 0);
    } else {
        qCWarning(imagelogging) << "Unknown mip format";
        Q_UNREACHABLE();
        return;
    }

    nvtt::OutputOptions outputOptions;
    outputOptions.setOutputHeader(false);
    MyOutputHandler outputHandler(texture, face);
    outputOptions.setOutputHandler(&outputHandler);
    MyErrorHandler errorHandler;
    outputOptions.setErrorHandler(&errorHandler);

    nvtt::Compressor compressor;
    compressor.process(inputOptions, compressionOptions, outputOptions);
#else
    texture->autoGenerateMips(-1);
#endif
}

void processTextureAlpha(const QImage& srcImage, bool& validAlpha, bool& alphaAsMask) {
    PROFILE_RANGE(resource_parse, "processTextureAlpha");
    validAlpha = false;
    alphaAsMask = true;
    const uint8 OPAQUE_ALPHA = 255;
    const uint8 TRANSPARENT_ALPHA = 0;

    // Figure out if we can use a mask for alpha or not
    int numOpaques = 0;
    int numTranslucents = 0;
    const int NUM_PIXELS = srcImage.width() * srcImage.height();
    const int MAX_TRANSLUCENT_PIXELS_FOR_ALPHAMASK = (int)(0.05f * (float)(NUM_PIXELS));
    const QRgb* data = reinterpret_cast<const QRgb*>(srcImage.constBits());
    for (int i = 0; i < NUM_PIXELS; ++i) {
        auto alpha = qAlpha(data[i]);
        if (alpha == OPAQUE_ALPHA) {
            numOpaques++;
        } else if (alpha != TRANSPARENT_ALPHA) {
            if (++numTranslucents > MAX_TRANSLUCENT_PIXELS_FOR_ALPHAMASK) {
                alphaAsMask = false;
                break;
            }
        }
    }
    validAlpha = (numOpaques != NUM_PIXELS);
}

gpu::TexturePointer TextureUsage::process2DTextureColorFromImage(const QImage& srcImage, const std::string& srcImageName, bool isStrict) {
    PROFILE_RANGE(resource_parse, "process2DTextureColorFromImage");
    QImage image = processSourceImage(srcImage, false);
    bool validAlpha = image.hasAlphaChannel();
    bool alphaAsMask = false;

    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    if (validAlpha) {
        processTextureAlpha(image, validAlpha, alphaAsMask);
    }

    gpu::TexturePointer theTexture = nullptr;

    if ((image.width() > 0) && (image.height() > 0)) {
        gpu::Element formatMip;
        gpu::Element formatGPU;
        if (isColorTexturesCompressionEnabled()) {
            if (validAlpha) {
                // NOTE: This disables BC1a compression because it was producing odd artifacts on text textures
                // for the tutorial. Instead we use BC3 (which is larger) but doesn't produce the same artifacts).
                formatGPU = gpu::Element::COLOR_COMPRESSED_SRGBA;
            } else {
                formatGPU = gpu::Element::COLOR_COMPRESSED_SRGB;
            }
            formatMip = formatGPU;
        } else {
            formatMip = gpu::Element::COLOR_SBGRA_32;
            formatGPU = gpu::Element::COLOR_SRGBA_32;
        }

        if (isStrict) {
            theTexture = gpu::Texture::createStrict(formatGPU, image.width(), image.height(), gpu::Texture::MAX_NUM_MIPS, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
        } else {
            theTexture = gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Texture::MAX_NUM_MIPS, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
        }
        theTexture->setSource(srcImageName);
        auto usage = gpu::Texture::Usage::Builder().withColor();
        if (validAlpha) {
            usage.withAlpha();
            if (alphaAsMask) {
                usage.withAlphaMask();
            }
        }
        theTexture->setUsage(usage.build());
        theTexture->setStoredMipFormat(formatMip);
        generateMips(theTexture.get(), image);
    }

    return theTexture;
}

int clampPixelCoordinate(int coordinate, int maxCoordinate) {
    return coordinate - ((int)(coordinate < 0) * coordinate) + ((int)(coordinate > maxCoordinate) * (maxCoordinate - coordinate));
}

const int RGBA_MAX = 255;

// transform -1 - 1 to 0 - 255 (from sobel value to rgb)
double mapComponent(double sobelValue) {
    const double factor = RGBA_MAX / 2.0;
    return (sobelValue + 1.0) * factor;
}

QImage processBumpMap(QImage& image) {
    if (image.format() != QImage::Format_Grayscale8) {
        image = image.convertToFormat(QImage::Format_Grayscale8);
    }

    // PR 5540 by AlessandroSigna integrated here as a specialized TextureLoader for bumpmaps
    // The conversion is done using the Sobel Filter to calculate the derivatives from the grayscale image
    const double pStrength = 2.0;
    int width = image.width();
    int height = image.height();

    QImage result(width, height, QImage::Format_ARGB32);

    for (int i = 0; i < width; i++) {
        const int iNextClamped = clampPixelCoordinate(i + 1, width - 1);
        const int iPrevClamped = clampPixelCoordinate(i - 1, width - 1);

        for (int j = 0; j < height; j++) {
            const int jNextClamped = clampPixelCoordinate(j + 1, height - 1);
            const int jPrevClamped = clampPixelCoordinate(j - 1, height - 1);

            // surrounding pixels
            const QRgb topLeft = image.pixel(iPrevClamped, jPrevClamped);
            const QRgb top = image.pixel(iPrevClamped, j);
            const QRgb topRight = image.pixel(iPrevClamped, jNextClamped);
            const QRgb right = image.pixel(i, jNextClamped);
            const QRgb bottomRight = image.pixel(iNextClamped, jNextClamped);
            const QRgb bottom = image.pixel(iNextClamped, j);
            const QRgb bottomLeft = image.pixel(iNextClamped, jPrevClamped);
            const QRgb left = image.pixel(i, jPrevClamped);

            // take their gray intensities
            // since it's a grayscale image, the value of each component RGB is the same
            const double tl = qRed(topLeft);
            const double t = qRed(top);
            const double tr = qRed(topRight);
            const double r = qRed(right);
            const double br = qRed(bottomRight);
            const double b = qRed(bottom);
            const double bl = qRed(bottomLeft);
            const double l = qRed(left);

            // apply the sobel filter
            const double dX = (tr + pStrength * r + br) - (tl + pStrength * l + bl);
            const double dY = (bl + pStrength * b + br) - (tl + pStrength * t + tr);
            const double dZ = RGBA_MAX / pStrength;

            glm::vec3 v(dX, dY, dZ);
            glm::normalize(v);

            // convert to rgb from the value obtained computing the filter
            QRgb qRgbValue = qRgba(mapComponent(v.z), mapComponent(v.y), mapComponent(v.x), 1.0);
            result.setPixel(i, j, qRgbValue);
        }
    }

    return result;
}
gpu::TexturePointer TextureUsage::process2DTextureNormalMapFromImage(const QImage& srcImage, const std::string& srcImageName, bool isBumpMap) {
    PROFILE_RANGE(resource_parse, "process2DTextureNormalMapFromImage");
    QImage image = processSourceImage(srcImage, false);

    if (isBumpMap) {
        image = processBumpMap(image);
    }

    // Make sure the normal map source image is ARGB32
    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    gpu::TexturePointer theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {
        gpu::Element formatMip = gpu::Element::VEC2NU8_XY;
        gpu::Element formatGPU = gpu::Element::VEC2NU8_XY;
        if (isNormalTexturesCompressionEnabled()) {
            formatMip = gpu::Element::COLOR_COMPRESSED_XY;
            formatGPU = gpu::Element::COLOR_COMPRESSED_XY;
        }

        theTexture = gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Texture::MAX_NUM_MIPS, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
        theTexture->setSource(srcImageName);
        theTexture->setStoredMipFormat(formatMip);
        generateMips(theTexture.get(), image);
    }

    return theTexture;
}

gpu::TexturePointer TextureUsage::process2DTextureGrayscaleFromImage(const QImage& srcImage, const std::string& srcImageName, bool isInvertedPixels) {
    PROFILE_RANGE(resource_parse, "process2DTextureGrayscaleFromImage");
    QImage image = processSourceImage(srcImage, false);

    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    if (isInvertedPixels) {
        // Gloss turned into Rough
        image.invertPixels(QImage::InvertRgba);
    }

    gpu::TexturePointer theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {
        gpu::Element formatMip;
        gpu::Element formatGPU;
        if (isGrayscaleTexturesCompressionEnabled()) {
            formatMip = gpu::Element::COLOR_COMPRESSED_RED;
            formatGPU = gpu::Element::COLOR_COMPRESSED_RED;
        } else {
            formatMip = gpu::Element::COLOR_R_8;
            formatGPU = gpu::Element::COLOR_R_8;
        }

        theTexture = gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Texture::MAX_NUM_MIPS, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
        theTexture->setSource(srcImageName);
        theTexture->setStoredMipFormat(formatMip);
        generateMips(theTexture.get(), image);
    }

    return theTexture;  
}

class CubeLayout {
public:

    enum SourceProjection {
        FLAT = 0,
        EQUIRECTANGULAR,
    };
    int _type = FLAT;
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
        _type(FLAT),
        _widthRatio(wr),
        _heightRatio(hr),
        _faceXPos(fXP),
        _faceXNeg(fXN),
        _faceYPos(fYP),
        _faceYNeg(fYN),
        _faceZPos(fZP),
        _faceZNeg(fZN) {}

    CubeLayout(int wr, int hr) :
        _type(EQUIRECTANGULAR),
        _widthRatio(wr),
        _heightRatio(hr) {}


    static const CubeLayout CUBEMAP_LAYOUTS[];
    static const int NUM_CUBEMAP_LAYOUTS;

    static int findLayout(int width, int height) {
        // Find the layout of the cubemap in the 2D image
        int foundLayout = -1;
        for (int i = 0; i < NUM_CUBEMAP_LAYOUTS; i++) {
            if ((height * CUBEMAP_LAYOUTS[i]._widthRatio) == (width * CUBEMAP_LAYOUTS[i]._heightRatio)) {
                foundLayout = i;
                break;
            }
        }
        return foundLayout;
    }

    static QImage extractEquirectangularFace(const QImage& source, gpu::Texture::CubeFace face, int faceWidth) {
        QImage image(faceWidth, faceWidth, source.format());

        glm::vec2 dstInvSize(1.0f / (float)image.width(), 1.0f / (float)image.height());

        struct CubeToXYZ {
            gpu::Texture::CubeFace _face;
            CubeToXYZ(gpu::Texture::CubeFace face) : _face(face) {}

            glm::vec3 xyzFrom(const glm::vec2& uv) {
                auto faceDir = glm::normalize(glm::vec3(-1.0f + 2.0f * uv.x, -1.0f + 2.0f * uv.y, 1.0f));

                switch (_face) {
                    case gpu::Texture::CubeFace::CUBE_FACE_BACK_POS_Z:
                        return glm::vec3(-faceDir.x, faceDir.y, faceDir.z);
                    case gpu::Texture::CubeFace::CUBE_FACE_FRONT_NEG_Z:
                        return glm::vec3(faceDir.x, faceDir.y, -faceDir.z);
                    case gpu::Texture::CubeFace::CUBE_FACE_LEFT_NEG_X:
                        return glm::vec3(faceDir.z, faceDir.y, faceDir.x);
                    case gpu::Texture::CubeFace::CUBE_FACE_RIGHT_POS_X:
                        return glm::vec3(-faceDir.z, faceDir.y, -faceDir.x);
                    case gpu::Texture::CubeFace::CUBE_FACE_BOTTOM_NEG_Y:
                        return glm::vec3(-faceDir.x, -faceDir.z, faceDir.y);
                    case gpu::Texture::CubeFace::CUBE_FACE_TOP_POS_Y:
                    default:
                        return glm::vec3(-faceDir.x, faceDir.z, -faceDir.y);
                }
            }
        };
        CubeToXYZ cubeToXYZ(face);

        struct RectToXYZ {
            RectToXYZ() {}

            glm::vec2 uvFrom(const glm::vec3& xyz) {
                auto flatDir = glm::normalize(glm::vec2(xyz.x, xyz.z));
                auto uvRad = glm::vec2(atan2(flatDir.x, flatDir.y), asin(xyz.y));

                const float LON_TO_RECT_U = 1.0f / (glm::pi<float>());
                const float LAT_TO_RECT_V = 2.0f / glm::pi<float>();
                return glm::vec2(0.5f * uvRad.x * LON_TO_RECT_U + 0.5f, 0.5f * uvRad.y * LAT_TO_RECT_V + 0.5f);
            }
        };
        RectToXYZ rectToXYZ;

        int srcFaceHeight = source.height();
        int srcFaceWidth = source.width();

        glm::vec2 dstCoord;
        glm::ivec2 srcPixel;
        for (int y = 0; y < faceWidth; ++y) {
            dstCoord.y = 1.0f - (y + 0.5f) * dstInvSize.y; // Fill cube face images from top to bottom
            for (int x = 0; x < faceWidth; ++x) {
                dstCoord.x = (x + 0.5f) * dstInvSize.x;

                auto xyzDir = cubeToXYZ.xyzFrom(dstCoord);
                auto srcCoord = rectToXYZ.uvFrom(xyzDir);

                srcPixel.x = floor(srcCoord.x * srcFaceWidth);
                // Flip the vertical axis to QImage going top to bottom
                srcPixel.y = floor((1.0f - srcCoord.y) * srcFaceHeight);

                if (((uint32)srcPixel.x < (uint32)source.width()) && ((uint32)srcPixel.y < (uint32)source.height())) {
                    image.setPixel(x, y, source.pixel(QPoint(srcPixel.x, srcPixel.y)));

                    // Keep for debug, this is showing the dir as a color
                    //  glm::u8vec4 rgba((xyzDir.x + 1.0)*0.5 * 256, (xyzDir.y + 1.0)*0.5 * 256, (xyzDir.z + 1.0)*0.5 * 256, 256);
                    //  unsigned int val = 0xff000000 | (rgba.r) | (rgba.g << 8) | (rgba.b << 16);
                    //  image.setPixel(x, y, val);
                }
            }
        }
        return image;
    }
};

const CubeLayout CubeLayout::CUBEMAP_LAYOUTS[] = {

    // Here is the expected layout for the faces in an image with the 2/1 aspect ratio:
    // THis is detected as an Equirectangular projection
    //                   WIDTH
    //       <--------------------------->
    //    ^  +------+------+------+------+
    //    H  |      |      |      |      |
    //    E  |      |      |      |      |
    //    I  |      |      |      |      |
    //    G  +------+------+------+------+
    //    H  |      |      |      |      |
    //    T  |      |      |      |      |
    //    |  |      |      |      |      |
    //    v  +------+------+------+------+
    //
    //    FaceWidth = width = height / 6
    { 2, 1 },

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
    { 1, 6,
    { 0, 0, true, false },
    { 0, 1, true, false },
    { 0, 2, false, true },
    { 0, 3, false, true },
    { 0, 4, true, false },
    { 0, 5, true, false }
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
    { 4, 3,
    { 2, 1, true, false },
    { 0, 1, true, false },
    { 1, 0, false, true },
    { 1, 2, false, true },
    { 3, 1, true, false },
    { 1, 1, true, false }
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
    { 3, 4,
    { 2, 1, true, false },
    { 0, 1, true, false },
    { 1, 0, false, true },
    { 1, 2, false, true },
    { 1, 3, false, true },
    { 1, 1, true, false }
    }
};
const int CubeLayout::NUM_CUBEMAP_LAYOUTS = sizeof(CubeLayout::CUBEMAP_LAYOUTS) / sizeof(CubeLayout);

gpu::TexturePointer TextureUsage::processCubeTextureColorFromImage(const QImage& srcImage, const std::string& srcImageName, bool generateIrradiance) {
    PROFILE_RANGE(resource_parse, "processCubeTextureColorFromImage");

    gpu::TexturePointer theTexture = nullptr;
    if ((srcImage.width() > 0) && (srcImage.height() > 0)) {
        QImage image = processSourceImage(srcImage, true);
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }

        gpu::Element formatMip;
        gpu::Element formatGPU;
        if (isCubeTexturesCompressionEnabled()) {
            formatMip = gpu::Element::COLOR_COMPRESSED_SRGBA_HIGH;
            formatGPU = gpu::Element::COLOR_COMPRESSED_SRGBA_HIGH;
        } else {
            formatMip = gpu::Element::COLOR_SRGBA_32;
            formatGPU = gpu::Element::COLOR_SRGBA_32;
        }

        // Find the layout of the cubemap in the 2D image
        // Use the original image size since processSourceImage may have altered the size / aspect ratio
        int foundLayout = CubeLayout::findLayout(srcImage.width(), srcImage.height());

        std::vector<QImage> faces;
        // If found, go extract the faces as separate images
        if (foundLayout >= 0) {
            auto& layout = CubeLayout::CUBEMAP_LAYOUTS[foundLayout];
            if (layout._type == CubeLayout::FLAT) {
                int faceWidth = image.width() / layout._widthRatio;

                faces.push_back(image.copy(QRect(layout._faceXPos._x * faceWidth, layout._faceXPos._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceXPos._horizontalMirror, layout._faceXPos._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceXNeg._x * faceWidth, layout._faceXNeg._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceXNeg._horizontalMirror, layout._faceXNeg._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceYPos._x * faceWidth, layout._faceYPos._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceYPos._horizontalMirror, layout._faceYPos._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceYNeg._x * faceWidth, layout._faceYNeg._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceYNeg._horizontalMirror, layout._faceYNeg._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceZPos._x * faceWidth, layout._faceZPos._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceZPos._horizontalMirror, layout._faceZPos._verticalMirror));
                faces.push_back(image.copy(QRect(layout._faceZNeg._x * faceWidth, layout._faceZNeg._y * faceWidth, faceWidth, faceWidth)).mirrored(layout._faceZNeg._horizontalMirror, layout._faceZNeg._verticalMirror));
            } else if (layout._type == CubeLayout::EQUIRECTANGULAR) {
                // THe face width is estimated from the input image
                const int EQUIRECT_FACE_RATIO_TO_WIDTH = 4;
                const int EQUIRECT_MAX_FACE_WIDTH = 2048;
                int faceWidth = std::min(image.width() / EQUIRECT_FACE_RATIO_TO_WIDTH, EQUIRECT_MAX_FACE_WIDTH);
                for (int face = gpu::Texture::CUBE_FACE_RIGHT_POS_X; face < gpu::Texture::NUM_CUBE_FACES; face++) {
                    QImage faceImage = CubeLayout::extractEquirectangularFace(image, (gpu::Texture::CubeFace) face, faceWidth);
                    faces.push_back(faceImage);
                }
            }
        } else {
            qCDebug(imagelogging) << "Failed to find a known cube map layout from this image:" << QString(srcImageName.c_str());
            return nullptr;
        }

        // If the 6 faces have been created go on and define the true Texture
        if (faces.size() == gpu::Texture::NUM_FACES_PER_TYPE[gpu::Texture::TEX_CUBE]) {
            theTexture = gpu::Texture::createCube(formatGPU, faces[0].width(), gpu::Texture::MAX_NUM_MIPS, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR, gpu::Sampler::WRAP_CLAMP));
            theTexture->setSource(srcImageName);
            theTexture->setStoredMipFormat(formatMip);

            for (uint8 face = 0; face < faces.size(); ++face) {
                generateMips(theTexture.get(), faces[face], face);
            }

            // Generate irradiance while we are at it
            if (generateIrradiance) {
                PROFILE_RANGE(resource_parse, "generateIrradiance");
                auto irradianceTexture = gpu::Texture::createCube(gpu::Element::COLOR_SRGBA_32, faces[0].width(), gpu::Texture::MAX_NUM_MIPS, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR, gpu::Sampler::WRAP_CLAMP));
                irradianceTexture->setSource(srcImageName);
                irradianceTexture->setStoredMipFormat(gpu::Element::COLOR_SBGRA_32);
                for (uint8 face = 0; face < faces.size(); ++face) {
                    irradianceTexture->assignStoredMipFace(0, face, faces[face].byteCount(), faces[face].constBits());
                }

                irradianceTexture->generateIrradiance();

                auto irradiance = irradianceTexture->getIrradiance();
                theTexture->overrideIrradiance(irradiance);
            }
        }
    }

    return theTexture;
}

} // namespace image
