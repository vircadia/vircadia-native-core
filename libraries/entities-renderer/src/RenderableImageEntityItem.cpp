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

using namespace render;
using namespace render::entities;

ImageEntityRenderer::ImageEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

ImageEntityRenderer::~ImageEntityRenderer() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

bool ImageEntityRenderer::isTransparent() const {
    return Parent::isTransparent() || (_textureIsLoaded && _texture->getGPUTexture() && _texture->getGPUTexture()->getUsage().isAlpha()) || _alpha < 1.0f;
}

bool ImageEntityRenderer::needsRenderUpdate() const {
    bool textureLoadedChanged = resultWithReadLock<bool>([&] {
        return (!_textureIsLoaded && _texture && _texture->isLoaded());
    });

    if (textureLoadedChanged) {
        return true;
    }

    return Parent::needsRenderUpdate();
}

bool ImageEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    bool needsUpdate = resultWithReadLock<bool>([&] {
        if (_imageURL != entity->getImageURL()) {
            return true;
        }

        if (_emissive != entity->getEmissive()) {
            return true;
        }

        if (_keepAspectRatio != entity->getKeepAspectRatio()) {
            return true;
        }

        if (_billboardMode != entity->getBillboardMode()) {
            return true;
        }

        if (_subImage != entity->getSubImage()) {
            return true;
        }

        if (_color != entity->getColor()) {
            return true;
        }

        if (_alpha != entity->getAlpha()) {
            return true;
        }

        return false;
    });

    return needsUpdate;
}

void ImageEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    withWriteLock([&] {
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

        _emissive = entity->getEmissive();
        _keepAspectRatio = entity->getKeepAspectRatio();
        _billboardMode = entity->getBillboardMode();
        _subImage = entity->getSubImage();

        _color = entity->getColor();
        _alpha = entity->getAlpha();

        if (!_textureIsLoaded && _texture && _texture->isLoaded()) {
            _textureIsLoaded = true;
        }
    });

    void* key = (void*)this;
    AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [this, entity]() {
        withWriteLock([&] {
            _dimensions = entity->getScaledDimensions();
            updateModelTransformAndBound();
            _renderTransform = getModelTransform();
        });
    });
}

ShapeKey ImageEntityRenderer::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withoutCullFace().withDepthBias();
    if (isTransparent()) {
        builder.withTranslucent();
    }

    withReadLock([&] {
        if (_emissive) {
            builder.withUnlit();
        }

        if (_primitiveMode == PrimitiveMode::LINES) {
            builder.withWireframe();
        }
    });

    return builder.build();
}

void ImageEntityRenderer::doRender(RenderArgs* args) {
    NetworkTexturePointer texture;
    QRect subImage;
    glm::u8vec3 color;
    glm::vec3 dimensions;
    Transform transform;
    withReadLock([&] {
        texture = _texture;
        subImage = _subImage;
        color = _color;
        dimensions = _dimensions;
        transform = _renderTransform;
    });

    if (!_visible || !texture || !texture->isLoaded()) {
        return;
    }

    Q_ASSERT(args->_batch);
    gpu::Batch* batch = args->_batch;

    if (_billboardMode == BillboardMode::YAW) {
        //rotate about vertical to face the camera
        glm::vec3 dPosition = args->getViewFrustum().getPosition() - transform.getTranslation();
        // If x and z are 0, atan(x, z) is undefined, so default to 0 degrees
        float yawRotation = dPosition.x == 0.0f && dPosition.z == 0.0f ? 0.0f : glm::atan(dPosition.x, dPosition.z);
        glm::quat orientation = glm::quat(glm::vec3(0.0f, yawRotation, 0.0f));
        transform.setRotation(orientation);
    } else if (_billboardMode == BillboardMode::FULL) {
        glm::vec3 billboardPos = transform.getTranslation();
        glm::vec3 cameraPos = args->getViewFrustum().getPosition();
        // use the referencial from the avatar, y isn't always up
        glm::vec3 avatarUP = EntityTreeRenderer::getAvatarUp();
        // check to see if glm::lookAt will work / using glm::lookAt variable name
        glm::highp_vec3 s(glm::cross(billboardPos - cameraPos, avatarUP));

        // make sure s is not NaN for any component
        if (glm::length2(s) > 0.0f) {
            glm::quat rotation(conjugate(toQuat(glm::lookAt(cameraPos, billboardPos, avatarUP))));
            transform.setRotation(rotation);
        }
    }
    transform.postScale(dimensions);

    batch->setModelTransform(transform);
    batch->setResourceTexture(0, texture->getGPUTexture());

    float imageWidth = texture->getWidth();
    float imageHeight = texture->getHeight();

    QRect fromImage;
    if (subImage.width() <= 0) {
        fromImage.setX(0);
        fromImage.setWidth(imageWidth);
    } else {
        float scaleX = imageWidth / texture->getOriginalWidth();
        fromImage.setX(scaleX * subImage.x());
        fromImage.setWidth(scaleX * subImage.width());
    }

    if (subImage.height() <= 0) {
        fromImage.setY(0);
        fromImage.setHeight(imageHeight);
    } else {
        float scaleY = imageHeight / texture->getOriginalHeight();
        fromImage.setY(scaleY * subImage.y());
        fromImage.setHeight(scaleY * subImage.height());
    }

    float maxSize = glm::max(fromImage.width(), fromImage.height());
    float x = _keepAspectRatio ? fromImage.width() / (2.0f * maxSize) : 0.5f;
    float y = _keepAspectRatio ? -fromImage.height() / (2.0f * maxSize) : -0.5f;

    glm::vec2 topLeft(-x, -y);
    glm::vec2 bottomRight(x, y);
    glm::vec2 texCoordTopLeft((fromImage.x() + 0.5f) / imageWidth, (fromImage.y() + 0.5f) / imageHeight);
    glm::vec2 texCoordBottomRight((fromImage.x() + fromImage.width() - 0.5f) / imageWidth,
                                  (fromImage.y() + fromImage.height() - 0.5f) / imageHeight);

    glm::vec4 imageColor(toGlm(color), _alpha);

    DependencyManager::get<GeometryCache>()->renderQuad(
        *batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight,
        imageColor, _geometryId
    );

    batch->setResourceTexture(0, nullptr);
}