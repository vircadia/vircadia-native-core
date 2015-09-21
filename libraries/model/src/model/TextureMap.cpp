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

// TextureStorage
TextureStorage::TextureStorage()
{/* : Texture::Storage()//, 
  //  _gpuTexture(Texture::createFromStorage(this))*/
}

TextureStorage::~TextureStorage() {
}

void TextureStorage::reset(const QUrl& url, const TextureUsage& usage) {
    _imageUrl = url;
    _usage = usage;
}

void TextureStorage::resetTexture(gpu::Texture* texture) {
    _gpuTexture.reset(texture);
}

bool TextureStorage::isDefined() const {
    if (_gpuTexture) {
        return _gpuTexture->isDefined();
    } else {
        return false;
    }
}


void TextureMap::setTextureStorage(TextureStoragePointer& texStorage) {
    _textureStorage = texStorage;
}

bool TextureMap::isDefined() const {
    if (_textureStorage) {
        return _textureStorage->isDefined();
    } else {
        return false;
    }
}

gpu::TextureView TextureMap::getTextureView() const {
    if (_textureStorage) {
        return gpu::TextureView(_textureStorage->getGPUTexture(), 0);
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




gpu::Texture* TextureStorage::create2DTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    QImage image = srcImage;
 
    int imageArea = image.width() * image.height();
    
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
            qCDebug(modelLog) << "Image with alpha channel is completely opaque:" << QString(srcImageName.c_str());
            image = image.convertToFormat(QImage::Format_RGB888);
        }
        
        averageColor = QColor(redTotal / imageArea,
                              greenTotal / imageArea, blueTotal / imageArea, alphaTotal / imageArea);
        
        isTransparent = (translucentPixels >= imageArea / 2);
    }
    
    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {
        
        // bool isLinearRGB = true; //(_type == NORMAL_TEXTURE) || (_type == EMISSIVE_TEXTURE);
        bool isLinearRGB = false; //(_type == NORMAL_TEXTURE) || (_type == EMISSIVE_TEXTURE);
        
        gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::UINT8, (isLinearRGB ? gpu::RGB : gpu::SRGB));
        gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::UINT8, (isLinearRGB ? gpu::RGB : gpu::SRGB));
        if (image.hasAlphaChannel()) {
            formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, (isLinearRGB ? gpu::RGBA : gpu::SRGBA));
            formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, (isLinearRGB ? gpu::BGRA : gpu::SBGRA));
        }
        

            theTexture = (gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
            theTexture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
            theTexture->autoGenerateMips(-1);
    }
    
    return theTexture;
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

gpu::Texture* TextureStorage::createCubeTextureFromImage(const QImage& srcImage, const std::string& srcImageName) {
    QImage image = srcImage;
    
    int imageArea = image.width() * image.height();
    
    
    qCDebug(modelLog) << "Cube map size:" << QString(srcImageName.c_str()) << image.width() << image.height();
    
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
            qCDebug(modelLog) << "Image with alpha channel is completely opaque:" << QString(srcImageName.c_str());
            image = image.convertToFormat(QImage::Format_RGB888);
        }
        
        averageColor = QColor(redTotal / imageArea,
                              greenTotal / imageArea, blueTotal / imageArea, alphaTotal / imageArea);
        
        isTransparent = (translucentPixels >= imageArea / 2);
    }
    
    gpu::Texture* theTexture = nullptr;
    if ((image.width() > 0) && (image.height() > 0)) {
        
        // bool isLinearRGB = true; //(_type == NORMAL_TEXTURE) || (_type == EMISSIVE_TEXTURE);
        bool isLinearRGB = false; //(_type == NORMAL_TEXTURE) || (_type == EMISSIVE_TEXTURE);
        
        gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::UINT8, (isLinearRGB ? gpu::RGB : gpu::SRGB));
        gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::UINT8, (isLinearRGB ? gpu::RGB : gpu::SRGB));
        if (image.hasAlphaChannel()) {
            formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, (isLinearRGB ? gpu::RGBA : gpu::SRGBA));
            formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, (isLinearRGB ? gpu::BGRA : gpu::SBGRA));
        }
        

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
                qCDebug(modelLog) << "Failed to find a known cube map layout from this image:" << QString(srcImageName.c_str());
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
    }
    
    return theTexture;
}
