//
//  TextureMap.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 5/6/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TextureMap.h"

#include <QImage>
#include <QPainter>
#include <QDebug>

#include "ModelLogging.h"

using namespace model;
using namespace gpu;

// FIXME: Declare this to enable compression
//#define COMPRESS_TEXTURES


void TextureMap::setTextureSource(TextureSourcePointer& textureSource) {
    _textureSource = textureSource;
}

bool TextureMap::isDefined() const {
    if (_textureSource) {
        return _textureSource->isDefined();
    } else {
        return false;
    }
}

gpu::TextureView TextureMap::getTextureView() const {
    if (_textureSource) {
        return gpu::TextureView(_textureSource->getGPUTexture(), 0);
    } else {
        return gpu::TextureView();
    }
}

void TextureMap::setTextureTransform(const Transform& texcoordTransform) {
    _texcoordTransform = texcoordTransform;
}

void TextureMap::setLightmapOffsetScale(float offset, float scale) {
    _lightmapOffsetScale.x = offset;
    _lightmapOffsetScale.y = scale;
}

const QImage TextureUsage::process2DImageColor(const QImage& srcImage, bool& validAlpha, bool& alphaAsMask) {
    QImage image = srcImage;
    validAlpha = false;
    alphaAsMask = true;
    const uint8 OPAQUE_ALPHA = 255;
    const uint8 TRANSPARENT_ALPHA = 0;
    if (image.hasAlphaChannel()) {
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }
        // Count the opaque and transparent pixels
        int numOpaques = 0;
        int numTransparents = 0;
        int height = image.height();
        int width = image.width();
        for (int y = 0; y < height; ++y) {
            const QRgb* data = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            for (int x = 0; x < width; ++x) {
                auto alpha = qAlpha(data[x]);
                numOpaques += (int)(alpha == OPAQUE_ALPHA);
                numTransparents += (int)(alpha == TRANSPARENT_ALPHA);
            }
        }

        // If alpha was meaningfull refine
        auto totalNumPixels = height * width;
        if (numOpaques != totalNumPixels) {
            auto numTranslucents = totalNumPixels - numOpaques - numTransparents;
            alphaAsMask = ((numTranslucents / (double)totalNumPixels) < 0.05);
        }
    }

    if (!validAlpha && image.format() != QImage::Format_RGB888) {
        image = image.convertToFormat(QImage::Format_RGB888);
    }

    return image;
}

void TextureUsage::defineColorTexelFormats(gpu::Element& formatGPU, gpu::Element& formatMip, 
const QImage& image, bool isLinear, bool doCompress) {

#ifdef COMPRESS_TEXTURES
#else
    doCompress = false;
#endif

    if (image.hasAlphaChannel()) {
        gpu::Semantic gpuSemantic;
        gpu::Semantic mipSemantic;
        if (isLinear) {
            mipSemantic = gpu::BGRA;
            if (doCompress) {
                gpuSemantic = gpu::COMPRESSED_RGBA;
            } else {
                gpuSemantic = gpu::RGBA;
            }
        } else {
            mipSemantic = gpu::SBGRA;
            if (doCompress) {
                gpuSemantic = gpu::COMPRESSED_SRGBA;
            } else {
                gpuSemantic = gpu::SRGBA;
            }
        }
        formatGPU = gpu::Element(gpu::VEC4, gpu::NUINT8, gpuSemantic);
        formatMip = gpu::Element(gpu::VEC4, gpu::NUINT8, mipSemantic);
    } else {
        gpu::Semantic gpuSemantic;
        gpu::Semantic mipSemantic;
        if (isLinear) {
            mipSemantic = gpu::RGB;
            if (doCompress) {
                gpuSemantic = gpu::COMPRESSED_RGB;
            } else {
                gpuSemantic = gpu::RGB;
            }
        } else {
            mipSemantic = gpu::SRGB;
            if (doCompress) {
                gpuSemantic = gpu::COMPRESSED_SRGB;
            } else {
                gpuSemantic = gpu::SRGB;
            }
        }
        formatGPU = gpu::Element(gpu::VEC3, gpu::NUINT8, gpuSemantic);
        formatMip = gpu::Element(gpu::VEC3, gpu::NUINT8, mipSemantic);
    }
}

gpu::Texture* TextureUsage::process2DTextureColorFromImage(const QImage& srcImage, bool isLinear, bool doCompress, bool generateMips) {
    bool validAlpha = false;
    bool alphaAsMask = true;
    QImage image = process2DImageColor(srcImage, validAlpha, alphaAsMask);

    gpu::Texture* theTexture = nullptr;

    if ((image.width() > 0) && (image.height() > 0)) {
        gpu::Element formatGPU;
        gpu::Element formatMip;
        defineColorTexelFormats(formatGPU, formatMip, image, isLinear, doCompress);

        theTexture = (gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));

        auto usage = gpu::Texture::Usage::Builder().withColor();
        if (validAlpha) {
            usage.withAlpha();
            if (alphaAsMask) {
                usage.withAlphaMask();
            }
        }
        theTexture->setUsage(usage.build());

        theTexture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());

        if (generateMips) {
            theTexture->autoGenerateMips(-1);
        }
    }

    return theTexture;
}

gpu::Texture* TextureUsage::create2DTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, false, false, true);
}


gpu::Texture* TextureUsage::createAlbedoTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, false, true, true);
}

gpu::Texture* TextureUsage::createEmissiveTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, false, true, true);
}

gpu::Texture* TextureUsage::createLightmapTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return process2DTextureColorFromImage(srcImage, false, true, true);
}


gpu::Texture* TextureUsage::createNormalTextureFromNormalImage(const QImage& srcImage, const std::string& srcImageName) {
    QImage image = srcImage;

    if (image.format() != QImage::Format_RGB888) {
        image = image.convertToFormat(QImage::Format_RGB888);
    }

    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {
        
        gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB);
        gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB);

        theTexture = (gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
        theTexture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
        theTexture->autoGenerateMips(-1);
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

gpu::Texture* TextureUsage::createNormalTextureFromBumpImage(const QImage& srcImage, const std::string& srcImageName) {
    QImage image = srcImage;
    
    if (image.format() != QImage::Format_RGB888) {
        image = image.convertToFormat(QImage::Format_RGB888);
    }

    // PR 5540 by AlessandroSigna integrated here as a specialized TextureLoader for bumpmaps
    // The conversion is done using the Sobel Filter to calculate the derivatives from the grayscale image
    const double pStrength = 2.0;
    int width = image.width();
    int height = image.height();
    QImage result(width, height, QImage::Format_RGB888);
    
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
            QRgb qRgbValue = qRgb(mapComponent(v.x), mapComponent(v.y), mapComponent(v.z));
            result.setPixel(i, j, qRgbValue);
        }
    }

    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {

        gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB);
        gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB);

        theTexture = (gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
        theTexture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
        theTexture->autoGenerateMips(-1);
    }

    return theTexture;
}

gpu::Texture* TextureUsage::createRoughnessTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    QImage image = srcImage;
    if (!image.hasAlphaChannel()) {
        if (image.format() != QImage::Format_RGB888) {
            image = image.convertToFormat(QImage::Format_RGB888);
        }
    } else {
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }
    }

    image = image.convertToFormat(QImage::Format_Grayscale8);

    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {
#ifdef COMPRESS_TEXTURES
        gpu::Element formatGPU = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::COMPRESSED_R);
#else
        gpu::Element formatGPU = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::RGB);
#endif
        gpu::Element formatMip = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::RGB);

        theTexture = (gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
        theTexture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
        theTexture->autoGenerateMips(-1);

        // FIXME queue for transfer to GPU and block on completion
    }

    return theTexture;
}

gpu::Texture* TextureUsage::createRoughnessTextureFromGlossImage(const QImage& srcImage, const std::string& srcImageName) {
    QImage image = srcImage;
    if (!image.hasAlphaChannel()) {
        if (image.format() != QImage::Format_RGB888) {
            image = image.convertToFormat(QImage::Format_RGB888);
        }
    } else {
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }
    }

    // Gloss turned into Rough
    image.invertPixels(QImage::InvertRgba);
    
    image = image.convertToFormat(QImage::Format_Grayscale8);
    
    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {
        
#ifdef COMPRESS_TEXTURES
        gpu::Element formatGPU = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::COMPRESSED_R);
#else
        gpu::Element formatGPU = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::RGB);
#endif
        gpu::Element formatMip = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::RGB);

        theTexture = (gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
        theTexture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
        theTexture->autoGenerateMips(-1);
        
        // FIXME queue for transfer to GPU and block on completion
    }
    
    return theTexture;
}

gpu::Texture* TextureUsage::createMetallicTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    QImage image = srcImage;
    if (!image.hasAlphaChannel()) {
        if (image.format() != QImage::Format_RGB888) {
            image = image.convertToFormat(QImage::Format_RGB888);
        }
    } else {
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }
    }

    image = image.convertToFormat(QImage::Format_Grayscale8);

    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {

#ifdef COMPRESS_TEXTURES
        gpu::Element formatGPU = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::COMPRESSED_R);
#else
        gpu::Element formatGPU = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::RGB);
#endif
        gpu::Element formatMip = gpu::Element(gpu::SCALAR, gpu::NUINT8, gpu::RGB);

        theTexture = (gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
        theTexture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
        theTexture->autoGenerateMips(-1);

        // FIXME queue for transfer to GPU and block on completion
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

                if (((uint32) srcPixel.x < (uint32) source.width()) && ((uint32) srcPixel.y < (uint32) source.height())) {
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

gpu::Texture* TextureUsage::processCubeTextureColorFromImage(const QImage& srcImage, const std::string& srcImageName, bool isLinear, bool doCompress, bool generateMips, bool generateIrradiance) {

    bool validAlpha = false;
    bool alphaAsMask = true;
    QImage image = process2DImageColor(srcImage, validAlpha, alphaAsMask);

    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {

        gpu::Element formatGPU;
        gpu::Element formatMip;
        defineColorTexelFormats(formatGPU, formatMip, image, isLinear, doCompress);

        // Find the layout of the cubemap in the 2D image
        int foundLayout = CubeLayout::findLayout(image.width(), image.height());
        
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
            qCDebug(modelLog) << "Failed to find a known cube map layout from this image:" << QString(srcImageName.c_str());
            return nullptr;
        }

        // If the 6 faces have been created go on and define the true Texture
        if (faces.size() == gpu::Texture::NUM_FACES_PER_TYPE[gpu::Texture::TEX_CUBE]) {
            theTexture = gpu::Texture::createCube(formatGPU, faces[0].width(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR, gpu::Sampler::WRAP_CLAMP));
            int f = 0;
            for (auto& face : faces) {
                theTexture->assignStoredMipFace(0, formatMip, face.byteCount(), face.constBits(), f);
                f++;
            }

            if (generateMips) {
                theTexture->autoGenerateMips(-1);
            }

            // Generate irradiance while we are at it
            if (generateIrradiance) {
                theTexture->generateIrradiance();
            }
        }
    }

    return theTexture;
}

gpu::Texture* TextureUsage::createCubeTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    return processCubeTextureColorFromImage(srcImage, srcImageName, false, true, true, true);
}
