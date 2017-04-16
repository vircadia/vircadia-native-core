//
//  Image.h
//  image/src/image
//
//  Created by Clement Brisset on 4/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_image_Image_h
#define hifi_image_Image_h

#include <QVariant>

#include <gpu/Texture.h>

class QByteArray;
class QImage;

namespace image {

using TextureLoader = std::function<gpu::TexturePointer(const QImage&, const std::string&)>;

TextureLoader getTextureLoaderForType(gpu::TextureType type, const QVariantMap& options = QVariantMap());

gpu::TexturePointer processImage(const QByteArray& content, const std::string& url, int maxNumPixels, gpu::TextureType textureType);

namespace TextureUsage {

gpu::TexturePointer create2DTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createStrict2DTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createAlbedoTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createEmissiveTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createNormalTextureFromNormalImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createNormalTextureFromBumpImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createRoughnessTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createRoughnessTextureFromGlossImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createMetallicTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createCubeTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createCubeTextureFromImageWithoutIrradiance(const QImage& image, const std::string& srcImageName);
gpu::TexturePointer createLightmapTextureFromImage(const QImage& image, const std::string& srcImageName);

const QImage process2DImageColor(const QImage& srcImage, bool& validAlpha, bool& alphaAsMask);
gpu::TexturePointer process2DTextureColorFromImage(const QImage& srcImage, const std::string& srcImageName, bool isLinear, bool isStrict = false);
gpu::TexturePointer processCubeTextureColorFromImage(const QImage& srcImage, const std::string& srcImageName, bool isLinear, bool generateIrradiance);

} // namespace TextureUsage

} // namespace image

#endif // hifi_image_Image_h
