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
#include <QImage>
#include <nvtt/nvtt.h>

#include <gpu/Texture.h>

#include "ColorChannel.h"

class QByteArray;

namespace image {

extern const QImage::Format QIMAGE_HDRFORMAT;

std::function<gpu::uint32(const glm::vec3&)> getHDRPackingFunction();
std::function<glm::vec3(gpu::uint32)> getHDRUnpackingFunction();
void convertToFloat(const unsigned char* source, int width, int height, size_t srcLineByteStride, gpu::Element sourceFormat, 
                    glm::vec4* output, size_t outputLinePixelStride);
void convertFromFloat(unsigned char* output, int width, int height, size_t outputLineByteStride, gpu::Element outputFormat,
                      const glm::vec4* source, size_t srcLinePixelStride);
                      
namespace TextureUsage {

enum Type {
    DEFAULT_TEXTURE,
    STRICT_TEXTURE,
    ALBEDO_TEXTURE,
    NORMAL_TEXTURE,
    BUMP_TEXTURE,
    SPECULAR_TEXTURE,
    METALLIC_TEXTURE = SPECULAR_TEXTURE, // for now spec and metallic texture are the same, converted to grey
    ROUGHNESS_TEXTURE,
    GLOSS_TEXTURE,
    EMISSIVE_TEXTURE,
    SKY_TEXTURE,
    AMBIENT_TEXTURE,
    OCCLUSION_TEXTURE,
    SCATTERING_TEXTURE = OCCLUSION_TEXTURE,
    LIGHTMAP_TEXTURE,
    UNUSED_TEXTURE
};

using TextureLoader = std::function<gpu::TexturePointer(QImage&&, const std::string&, bool, gpu::BackendTarget, const std::atomic<bool>&)>;
TextureLoader getTextureLoaderForType(Type type, const QVariantMap& options = QVariantMap());

gpu::TexturePointer create2DTextureFromImage(QImage&& image, const std::string& srcImageName,
                                             bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createStrict2DTextureFromImage(QImage&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createAlbedoTextureFromImage(QImage&& image, const std::string& srcImageName,
                                                 bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createEmissiveTextureFromImage(QImage&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createNormalTextureFromNormalImage(QImage&& image, const std::string& srcImageName,
                                                       bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createNormalTextureFromBumpImage(QImage&& image, const std::string& srcImageName,
                                                     bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createRoughnessTextureFromImage(QImage&& image, const std::string& srcImageName,
                                                    bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createRoughnessTextureFromGlossImage(QImage&& image, const std::string& srcImageName,
                                                         bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createMetallicTextureFromImage(QImage&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createCubeTextureFromImage(QImage&& image, const std::string& srcImageName,
                                               bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createCubeTextureAndIrradianceFromImage(QImage&& image, const std::string& srcImageName,
                                                            bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createAmbientCubeTextureFromImage(QImage&& image, const std::string& srcImageName,
                                                      bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createAmbientCubeTextureAndIrradianceFromImage(QImage&& image, const std::string& srcImageName,
                                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createLightmapTextureFromImage(QImage&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing); 
gpu::TexturePointer process2DTextureColorFromImage(QImage&& srcImage, const std::string& srcImageName, bool compress,
                                                   gpu::BackendTarget target, bool isStrict, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer process2DTextureNormalMapFromImage(QImage&& srcImage, const std::string& srcImageName, bool compress,
                                                       gpu::BackendTarget target, bool isBumpMap, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer process2DTextureGrayscaleFromImage(QImage&& srcImage, const std::string& srcImageName, bool compress,
                                                       gpu::BackendTarget target, bool isInvertedPixels, const std::atomic<bool>& abortProcessing);

enum CubeTextureOptions {
    CUBE_DEFAULT = 0x0,
    CUBE_GENERATE_IRRADIANCE = 0x1,
    CUBE_GGX_CONVOLVE = 0x2
};
gpu::TexturePointer processCubeTextureColorFromImage(QImage&& srcImage, const std::string& srcImageName, bool compress,
                                                     gpu::BackendTarget target, int option, const std::atomic<bool>& abortProcessing);

} // namespace TextureUsage

const QStringList getSupportedFormats();

gpu::TexturePointer processImage(std::shared_ptr<QIODevice> content, const std::string& url, ColorChannel sourceChannel,
                                 int maxNumPixels, TextureUsage::Type textureType,
                                 bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing = false);

#if defined(NVTT_API)
class SequentialTaskDispatcher : public nvtt::TaskDispatcher {
public:
    SequentialTaskDispatcher(const std::atomic<bool>& abortProcessing);

    const std::atomic<bool>& _abortProcessing;

    void dispatch(nvtt::Task* task, void* context, int count) override;
};

nvtt::OutputHandler* getNVTTCompressionOutputHandler(gpu::Texture* outputTexture, int face, nvtt::CompressionOptions& compressOptions);
#endif
} // namespace image

#endif // hifi_image_Image_h
