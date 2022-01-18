//
//  Created by Sam Gondelman on 11/29/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableImageEntityItem.h"

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <graphics/ShaderConstants.h>

#include "RenderPipelines.h"

using namespace render;
using namespace render::entities;

ImageEntityRenderer::ImageEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
    _material->setCullFaceMode(graphics::MaterialKey::CullFaceMode::CULL_NONE);
    addMaterial(graphics::MaterialLayer(_material, 0), "0");
}

ImageEntityRenderer::~ImageEntityRenderer() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

bool ImageEntityRenderer::needsRenderUpdate() const {
    return needsRenderUpdateFromMaterials() || Parent::needsRenderUpdate();
}

void ImageEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    void* key = (void*)this;
    AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [this, entity] {
        withWriteLock([&] {
            _renderTransform = getModelTransform();
            _renderTransform.postScale(entity->getScaledDimensions());
        });
    });
}

void ImageEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    auto imageURL = entity->getImageURL();
    if (_imageURL != imageURL) {
        _imageURL = imageURL;
        if (imageURL.isEmpty()) {
            _texture.reset();
        } else {
            _texture = DependencyManager::get<TextureCache>()->getTexture(_imageURL);
        }
        _textureIsLoaded = false;
    }

    _keepAspectRatio = entity->getKeepAspectRatio();
    _subImage = entity->getSubImage();
    _pulseProperties = entity->getPulseProperties();

    bool materialChanged = false;
    glm::vec3 color = toGlm(entity->getColor());
    if (_color != color) {
        _color = color;
        _material->setAlbedo(color);
        materialChanged = true;
    }

    float alpha = entity->getAlpha();
    if (_alpha != alpha) {
        _alpha = alpha;
        _material->setOpacity(alpha);
        materialChanged = true;
    }

    auto emissive = entity->getEmissive();
    if (_emissive != emissive) {
        _emissive = emissive;
        _material->setUnlit(_emissive);
        materialChanged = true;
    }

    updateMaterials(materialChanged);

    bool nextTextureLoaded = _texture && (_texture->isLoaded() || _texture->isFailed());
    if (!_textureIsLoaded) {
        emit requestRenderUpdate();
        if (nextTextureLoaded) {
            float width = _texture->getOriginalWidth();
            float height = _texture->getOriginalHeight();
            glm::vec3 naturalDimensions = glm::vec3(1.0f, 1.0f, 0.01f);
            if (width < height) {
                naturalDimensions.x = width / height;
            } else {
                naturalDimensions.y = height / width;
            }
            // Unlike Models (where the Renderer also doubles as the EntityItem), Images need to
            // convey this information back to the game object from the Renderer
            entity->setNaturalDimension(naturalDimensions);
        }
    }
    _textureIsLoaded = nextTextureLoaded;
}

bool ImageEntityRenderer::isTransparent() const {
    bool imageTransparent = _alpha < 1.0f || _pulseProperties.getAlphaMode() != PulseMode::NONE ||
        (_textureIsLoaded && _texture->getGPUTexture() && _texture->getGPUTexture()->getUsage().isAlpha());
    return imageTransparent || Parent::isTransparent() || materialsTransparent();
}

Item::Bound ImageEntityRenderer::getBound(RenderArgs* args) {
    return Parent::getMaterialBound(args);
}

ShapeKey ImageEntityRenderer::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withDepthBias();
    updateShapeKeyBuilderFromMaterials(builder);
    return builder.build();
}

void ImageEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableImageEntityItem::render");
    Q_ASSERT(args->_batch);

    graphics::MultiMaterial materials;
    {
        std::lock_guard<std::mutex> lock(_materialsLock);
        materials = _materials["0"];
    }

    auto& schema = materials.getSchemaBuffer().get<graphics::MultiMaterial::Schema>();
    glm::vec4 color = glm::vec4(ColorUtils::tosRGBVec3(schema._albedo), schema._opacity);
    color = EntityRenderer::calculatePulseColor(color, _pulseProperties, _created);

    if (!_texture || !_texture->isLoaded() || color.a == 0.0f) {
        return;
    }

    Transform transform;
    bool transparent;
    withReadLock([&] {
        transform = _renderTransform;
        transparent = isTransparent();
    });

    gpu::Batch* batch = args->_batch;

    transform.setRotation(BillboardModeHelpers::getBillboardRotation(transform.getTranslation(), transform.getRotation(), _billboardMode,
        args->_renderMode == RenderArgs::RenderMode::SHADOW_RENDER_MODE ? BillboardModeHelpers::getPrimaryViewFrustumPosition() : args->getViewFrustum().getPosition()));

    float imageWidth = _texture->getWidth();
    float imageHeight = _texture->getHeight();
    float originalWidth = _texture->getOriginalWidth();
    float originalHeight = _texture->getOriginalHeight();

    QRect fromImage;
    if (_subImage.width() <= 0) {
        fromImage.setX(0);
        fromImage.setWidth(imageWidth);
    } else {
        float scaleX = imageWidth / originalWidth;
        fromImage.setX(scaleX * _subImage.x());
        fromImage.setWidth(scaleX * _subImage.width());
    }

    if (_subImage.height() <= 0) {
        fromImage.setY(0);
        fromImage.setHeight(imageHeight);
    } else {
        float scaleY = imageHeight / originalHeight;
        fromImage.setY(scaleY * _subImage.y());
        fromImage.setHeight(scaleY * _subImage.height());
    }

    glm::vec2 texCoordBottomLeft((fromImage.x() + 0.5f) / imageWidth, (fromImage.y() + fromImage.height() - 0.5f) / imageHeight);
    glm::vec2 texCoordTopRight((fromImage.x() + fromImage.width() - 0.5f) / imageWidth, (fromImage.y() + 0.5f) / imageHeight);

    if (_keepAspectRatio) {
        glm::vec3 scale = transform.getScale();
        float targetAspectRatio = originalWidth / originalHeight;
        float currentAspectRatio = scale.x / scale.y;

        if (targetAspectRatio < currentAspectRatio) {
            scale.x *= targetAspectRatio / currentAspectRatio;
        } else {
            scale.y /= targetAspectRatio / currentAspectRatio;
        }
        transform.setScale(scale);
    }
    batch->setModelTransform(transform);

    Pipeline pipelineType = getPipelineType(materials);
    if (pipelineType == Pipeline::PROCEDURAL) {
        auto procedural = std::static_pointer_cast<graphics::ProceduralMaterial>(materials.top().material);
        transparent |= procedural->isFading();
        procedural->prepare(*batch, transform.getTranslation(), transform.getScale(), transform.getRotation(), _created, ProceduralProgramKey(transparent));
    } else if (pipelineType == Pipeline::SIMPLE) {
        batch->setResourceTexture(0, _texture->getGPUTexture());
    } else if (RenderPipelines::bindMaterials(materials, *batch, args->_renderMode, args->_enableTexturing)) {
        args->_details._materialSwitches++;
    }

    DependencyManager::get<GeometryCache>()->renderQuad(
        *batch, glm::vec2(-0.5f), glm::vec2(0.5f), texCoordBottomLeft, texCoordTopRight,
        color, _geometryId
    );

    if (pipelineType == Pipeline::SIMPLE) {
        // we have to reset this to white for other simple shapes
        batch->setResourceTexture(graphics::slot::texture::Texture::MaterialAlbedo, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }
}
