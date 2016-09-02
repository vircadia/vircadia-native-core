//
//  TextureMap.h
//  libraries/model/src/model
//
//  Created by Sam Gateau on 5/6/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_TextureMap_h
#define hifi_model_TextureMap_h

#include "gpu/Texture.h"

#include "Material.h"
#include "Transform.h"

#include <qurl.h>

class QImage;

namespace model {

typedef glm::vec3 Color;

class TextureUsage {
public:
    gpu::Texture::Type _type{ gpu::Texture::TEX_2D };
    Material::MapFlags _materialUsage{ MaterialKey::ALBEDO_MAP };

    int _environmentUsage = 0;

    static gpu::Texture* create2DTextureFromImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createAlbedoTextureFromImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createEmissiveTextureFromImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createNormalTextureFromNormalImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createNormalTextureFromBumpImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createRoughnessTextureFromImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createRoughnessTextureFromGlossImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createMetallicTextureFromImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createCubeTextureFromImage(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createCubeTextureFromImageWithoutIrradiance(const QImage& image, const std::string& srcImageName);
    static gpu::Texture* createLightmapTextureFromImage(const QImage& image, const std::string& srcImageName);


    static const QImage process2DImageColor(const QImage& srcImage, bool& validAlpha, bool& alphaAsMask);
    static void defineColorTexelFormats(gpu::Element& formatGPU, gpu::Element& formatMip,
        const QImage& srcImage, bool isLinear, bool doCompress);
    static gpu::Texture* process2DTextureColorFromImage(const QImage& srcImage, bool isLinear, bool doCompress, bool generateMips);
    static gpu::Texture* processCubeTextureColorFromImage(const QImage& srcImage, const std::string& srcImageName, bool isLinear, bool doCompress, bool generateMips, bool generateIrradiance);

};



class TextureMap {
public:
    TextureMap() {}

    void setTextureSource(gpu::TextureSourcePointer& textureSource);
    gpu::TextureSourcePointer getTextureSource() const { return _textureSource; }

    bool isDefined() const;
    gpu::TextureView getTextureView() const;

    void setTextureTransform(const Transform& texcoordTransform);
    const Transform& getTextureTransform() const { return _texcoordTransform; }

    void setUseAlphaChannel(bool useAlpha) { _useAlphaChannel = useAlpha; }
    bool useAlphaChannel() const { return _useAlphaChannel; }

    void setLightmapOffsetScale(float offset, float scale);
    const glm::vec2& getLightmapOffsetScale() const { return _lightmapOffsetScale; }

protected:
    gpu::TextureSourcePointer _textureSource;

    Transform _texcoordTransform;
    glm::vec2 _lightmapOffsetScale{ 0.0f, 1.0f };

    bool _useAlphaChannel{ false };
};
typedef std::shared_ptr< TextureMap > TextureMapPointer;

};

#endif // hifi_model_TextureMap_h

