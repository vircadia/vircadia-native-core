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

using TextureLoader = std::function<gpu::Texture*(const QImage&, const std::string&)>;

TextureLoader getTextureLoaderForType(gpu::TextureType type, const QVariantMap& options = QVariantMap());

gpu::Texture* processImage(const QByteArray& content, const std::string& url, int maxNumPixels, gpu::TextureType textureType);

namespace TextureUsage {

gpu::Texture* create2DTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createStrict2DTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createAlbedoTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createEmissiveTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createNormalTextureFromNormalImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createNormalTextureFromBumpImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createRoughnessTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createRoughnessTextureFromGlossImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createMetallicTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createCubeTextureFromImage(const QImage& image, const std::string& srcImageName);
gpu::Texture* createCubeTextureFromImageWithoutIrradiance(const QImage& image, const std::string& srcImageName);
gpu::Texture* createLightmapTextureFromImage(const QImage& image, const std::string& srcImageName);

const QImage process2DImageColor(const QImage& srcImage, bool& validAlpha, bool& alphaAsMask);
gpu::Texture* process2DTextureColorFromImage(const QImage& srcImage, const std::string& srcImageName, bool isLinear, bool isStrict = false);
gpu::Texture* processCubeTextureColorFromImage(const QImage& srcImage, const std::string& srcImageName, bool isLinear, bool generateIrradiance);

} // namespace TextureUsage

} // namespace image

#endif // hifi_image_Image_h
