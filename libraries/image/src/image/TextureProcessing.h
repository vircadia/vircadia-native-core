//
//  TextureProcessing.h
//  image/src/TextureProcessing
//
//  Created by Clement Brisset on 4/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_image_TextureProcessing_h
#define hifi_image_TextureProcessing_h

#include <QVariant>

#include <gpu/Texture.h>

#include "Image.h"
#include <nvtt/nvtt.h>

namespace image {

    std::function<gpu::uint32(const glm::vec3&)> getHDRPackingFunction();
    std::function<glm::vec3(gpu::uint32)> getHDRUnpackingFunction();
    void convertToFloatFromPacked(const unsigned char* source, int width, int height, size_t srcLineByteStride, gpu::Element sourceFormat,
                        glm::vec4* output, size_t outputLinePixelStride);
    void convertToPackedFromFloat(unsigned char* output, int width, int height, size_t outputLineByteStride, gpu::Element outputFormat,
                          const glm::vec4* source, size_t srcLinePixelStride);

namespace TextureUsage {

/*@jsdoc
 * <p>Describes the type of texture.</p>
 * <p>See also: {@link Material} and
 * {@link https://docs.vircadia.com/create/3d-models/pbr-materials-guide.html|PBR Materials Guide}.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Name</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code></td><td>Default</td><td>Basic color.</td></tr>
 *     <tr><td><code>1</code></td><td>Strict</td><td>Basic color. Quality never downgraded.</td></tr>
 *     <tr><td><code>2</code></td><td>Albedo</td><td>Color for PBR.</td></tr>
 *     <tr><td><code>3</code></td><td>Normal</td><td>Normal map.</td></tr>
 *     <tr><td><code>4</code></td><td>Bump</td><td>Bump map.</td></tr>
 *     <tr><td><code>5</code></td><td>Specular or metallic</td><td>Metallic or not.</td></tr>
 *     <tr><td><code>6</code></td><td>Roughness</td><td>Rough or matte.</td></tr>
 *     <tr><td><code>7</code></td><td>Gloss</td><td>Gloss or shine.</td></tr>
 *     <tr><td><code>8</code></td><td>Emissive</td><td>The amount of light reflected.</td></tr>
 *     <tr><td><code>9</code></td><td>Cube</td><td>Cubic image for sky boxes.</td></tr>
 *     <tr><td><code>10</code></td><td>Occlusion or scattering</td><td>How objects or human skin interact with light.</td></tr>
 *     <tr><td><code>11</code></td><td>Lightmap</td><td>Light map.</td></tr>
 *     <tr><td><code>12</code></td><td>Unused</td><td>Texture is not currently used.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} TextureCache.TextureType
 */
enum Type {
    // NOTE: add new texture types at the bottom here
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

using TextureLoader = std::function<gpu::TexturePointer(Image&&, const std::string&, bool, gpu::BackendTarget, const std::atomic<bool>&)>;
TextureLoader getTextureLoaderForType(Type type);

gpu::TexturePointer create2DTextureFromImage(Image&& image, const std::string& srcImageName,
                                             bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createStrict2DTextureFromImage(Image&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createAlbedoTextureFromImage(Image&& image, const std::string& srcImageName,
                                                 bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createEmissiveTextureFromImage(Image&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createNormalTextureFromNormalImage(Image&& image, const std::string& srcImageName,
                                                       bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createNormalTextureFromBumpImage(Image&& image, const std::string& srcImageName,
                                                     bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createRoughnessTextureFromImage(Image&& image, const std::string& srcImageName,
                                                    bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createRoughnessTextureFromGlossImage(Image&& image, const std::string& srcImageName,
                                                         bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createMetallicTextureFromImage(Image&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createCubeTextureFromImage(Image&& image, const std::string& srcImageName,
                                               bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createAmbientCubeTextureAndIrradianceFromImage(Image&& image, const std::string& srcImageName,
                                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createLightmapTextureFromImage(Image&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer process2DTextureColorFromImage(Image&& srcImage, const std::string& srcImageName, bool compress,
                                                   gpu::BackendTarget target, bool isStrict, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer process2DTextureNormalMapFromImage(Image&& srcImage, const std::string& srcImageName, bool compress,
                                                       gpu::BackendTarget target, bool isBumpMap, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer process2DTextureGrayscaleFromImage(Image&& srcImage, const std::string& srcImageName, bool compress,
                                                       gpu::BackendTarget target, bool isInvertedPixels, const std::atomic<bool>& abortProcessing);

enum CubeTextureOptions {
    CUBE_DEFAULT = 0x0,
    CUBE_GENERATE_IRRADIANCE = 0x1,
    CUBE_GGX_CONVOLVE = 0x2
};
gpu::TexturePointer processCubeTextureColorFromImage(Image&& srcImage, const std::string& srcImageName, bool compress,
                                                     gpu::BackendTarget target, int option, const std::atomic<bool>& abortProcessing);
} // namespace TextureUsage

const QStringList getSupportedFormats();

std::pair<gpu::TexturePointer, glm::ivec2> processImage(std::shared_ptr<QIODevice> content, const std::string& url, ColorChannel sourceChannel,
                                                        int maxNumPixels, TextureUsage::Type textureType,
                                                        bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing = false);

void convertToTextureWithMips(gpu::Texture* texture, Image&& image, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing = false, int face = -1);
void convertToTexture(gpu::Texture* texture, Image&& image, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing = false, int face = -1, int mipLevel = 0);

} // namespace image

#endif // hifi_image_TextureProcessing_h
