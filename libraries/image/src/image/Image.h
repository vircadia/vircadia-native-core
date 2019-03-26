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

#include "ColorChannel.h"

class QByteArray;
class QImage;

namespace image {

namespace TextureUsage {

/**jsdoc
 * <p>Signifies what type of texture a texture is.</p>
 * <p>See also: {@link Material} and 
 * {@link https://docs.highfidelity.com/create/3d-models/pbr-materials-guide.html|PBR Materials Guide}.</p>
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
 *     <tr><td><code>5</code></td><td>Specular / Metallic</td><td>Metallic or not.</td></tr>
 *     <tr><td><code>6</code></td><td>Roughness</td><td>Rough / matte.</td></tr>
 *     <tr><td><code>7</code></td><td>Gloss</td><td>Gloss / shine.</td></tr>
 *     <tr><td><code>8</code></td><td>Emissive</td><td>The amount of light reflected.</td></tr>
 *     <tr><td><code>9</code></td><td>Cube</td><td>Cubic image for sky boxes.</td></tr>
 *     <tr><td><code>10</code></td><td>Occlusion / Scattering</td><td>How objects / human skin interact with light.</td></tr>
 *     <tr><td><code>11</code></td><td>Lightmap</td><td>Light map.</td></tr>
 *     <tr><td><code>12</code></td><td>Unused</td><td>Texture is not used.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} TextureCache.TextureType
 */
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
    CUBE_TEXTURE,
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
gpu::TexturePointer createCubeTextureFromImageWithoutIrradiance(QImage&& image, const std::string& srcImageName,
                                                                bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer createLightmapTextureFromImage(QImage&& image, const std::string& srcImageName,
                                                   bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing); 
gpu::TexturePointer process2DTextureColorFromImage(QImage&& srcImage, const std::string& srcImageName, bool compress,
                                                   gpu::BackendTarget target, bool isStrict, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer process2DTextureNormalMapFromImage(QImage&& srcImage, const std::string& srcImageName, bool compress,
                                                       gpu::BackendTarget target, bool isBumpMap, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer process2DTextureGrayscaleFromImage(QImage&& srcImage, const std::string& srcImageName, bool compress,
                                                       gpu::BackendTarget target, bool isInvertedPixels, const std::atomic<bool>& abortProcessing);
gpu::TexturePointer processCubeTextureColorFromImage(QImage&& srcImage, const std::string& srcImageName, bool compress,
                                                     gpu::BackendTarget target, bool generateIrradiance, const std::atomic<bool>& abortProcessing);

} // namespace TextureUsage

const QStringList getSupportedFormats();

gpu::TexturePointer processImage(std::shared_ptr<QIODevice> content, const std::string& url, ColorChannel sourceChannel,
                                 int maxNumPixels, TextureUsage::Type textureType,
                                 bool compress, gpu::BackendTarget target, const std::atomic<bool>& abortProcessing = false);

} // namespace image

#endif // hifi_image_Image_h
