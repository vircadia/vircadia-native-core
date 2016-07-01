//
//  Image3DOverlay.cpp
//
//
//  Created by Clement on 7/1/14.
//  Modified and renamed by Zander Otavka on 8/4/15
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Image3DOverlay.h"

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <gpu/Batch.h>
#include <RegisteredMetaTypes.h>

#include "GeometryUtil.h"


QString const Image3DOverlay::TYPE = "image3d";

Image3DOverlay::Image3DOverlay() {
    _isLoaded = false;
    _emissive = false;
}

Image3DOverlay::Image3DOverlay(const Image3DOverlay* image3DOverlay) :
    Billboard3DOverlay(image3DOverlay),
    _url(image3DOverlay->_url),
    _texture(image3DOverlay->_texture),
    _emissive(image3DOverlay->_emissive),
    _fromImage(image3DOverlay->_fromImage)
{
}

void Image3DOverlay::update(float deltatime) {
    applyTransformTo(_transform);
}

void Image3DOverlay::render(RenderArgs* args) {
    if (!_isLoaded) {
        _isLoaded = true;
        _texture = DependencyManager::get<TextureCache>()->getTexture(_url);
    }

    if (!_visible || !getParentVisible() || !_texture || !_texture->isLoaded()) {
        return;
    }

    Q_ASSERT(args->_batch);
    gpu::Batch* batch = args->_batch;

    float imageWidth = _texture->getWidth();
    float imageHeight = _texture->getHeight();

    QRect fromImage;
    if (_fromImage.isNull()) {
        fromImage.setX(0);
        fromImage.setY(0);
        fromImage.setWidth(imageWidth);
        fromImage.setHeight(imageHeight);
    } else {
        float scaleX = imageWidth / _texture->getOriginalWidth();
        float scaleY = imageHeight / _texture->getOriginalHeight();

        fromImage.setX(scaleX * _fromImage.x());
        fromImage.setY(scaleY * _fromImage.y());
        fromImage.setWidth(scaleX * _fromImage.width());
        fromImage.setHeight(scaleY * _fromImage.height());
    }

    float maxSize = glm::max(fromImage.width(), fromImage.height());
    float x = fromImage.width() / (2.0f * maxSize);
    float y = -fromImage.height() / (2.0f * maxSize);

    glm::vec2 topLeft(-x, -y);
    glm::vec2 bottomRight(x, y);
    glm::vec2 texCoordTopLeft(fromImage.x() / imageWidth, fromImage.y() / imageHeight);
    glm::vec2 texCoordBottomRight((fromImage.x() + fromImage.width()) / imageWidth,
                                  (fromImage.y() + fromImage.height()) / imageHeight);

    const float MAX_COLOR = 255.0f;
    xColor color = getColor();
    float alpha = getAlpha();

    applyTransformTo(_transform, true);
    Transform transform = _transform;
    transform.postScale(glm::vec3(getDimensions(), 1.0f));

    batch->setModelTransform(transform);
    batch->setResourceTexture(0, _texture->getGPUTexture());
    
    DependencyManager::get<GeometryCache>()->renderQuad(
        *batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight,
        glm::vec4(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha)
    );

    batch->setResourceTexture(0, args->_whiteTexture); // restore default white color after me
}

const render::ShapeKey Image3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withoutCullFace().withDepthBias();
    if (_emissive) {
        builder.withUnlit();
    }
    if (getAlpha() != 1.0f) {
        builder.withTranslucent();
    }
    return builder.build();
}

void Image3DOverlay::setProperties(const QVariantMap& properties) {
    Billboard3DOverlay::setProperties(properties);

    auto urlValue = properties["url"];
    if (urlValue.isValid()) {
        QString newURL = urlValue.toString();
        if (newURL != _url) {
            setURL(newURL);
        }
    }

    auto subImageBoundsVar = properties["subImage"];
    if (subImageBoundsVar.isValid()) {
        if (subImageBoundsVar.isNull()) {
            _fromImage = QRect();
        } else {
            QRect oldSubImageRect = _fromImage;
            QRect subImageRect = _fromImage;
            auto subImageBounds = subImageBoundsVar.toMap();
            if (subImageBounds["x"].isValid()) {
                subImageRect.setX(subImageBounds["x"].toInt());
            } else {
                subImageRect.setX(oldSubImageRect.x());
            }
            if (subImageBounds["y"].isValid()) {
                subImageRect.setY(subImageBounds["y"].toInt());
            } else {
                subImageRect.setY(oldSubImageRect.y());
            }
            if (subImageBounds["width"].isValid()) {
                subImageRect.setWidth(subImageBounds["width"].toInt());
            } else {
                subImageRect.setWidth(oldSubImageRect.width());
            }
            if (subImageBounds["height"].isValid()) {
                subImageRect.setHeight(subImageBounds["height"].toInt());
            } else {
                subImageRect.setHeight(oldSubImageRect.height());
            }
            setClipFromSource(subImageRect);
        }
    }

    auto emissiveValue = properties["emissive"];
    if (emissiveValue.isValid()) {
        _emissive = emissiveValue.toBool();
    }
}

QVariant Image3DOverlay::getProperty(const QString& property) {
    if (property == "url") {
        return _url;
    }
    if (property == "subImage") {
        return _fromImage;
    }
    if (property == "offsetPosition") {
        return vec3toVariant(getOffsetPosition());
    }
    if (property == "emissive") {
        return _emissive;
    }

    return Billboard3DOverlay::getProperty(property);
}

void Image3DOverlay::setURL(const QString& url) {
    _url = url;
    _isLoaded = false;
}

bool Image3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                            float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    if (_texture && _texture->isLoaded()) {
        // Make sure position and rotation is updated.
        applyTransformTo(_transform, true);

        // Produce the dimensions of the overlay based on the image's aspect ratio and the overlay's scale.
        bool isNull = _fromImage.isNull();
        float width = isNull ? _texture->getWidth() : _fromImage.width();
        float height = isNull ? _texture->getHeight() : _fromImage.height();
        float maxSize = glm::max(width, height);
        glm::vec2 dimensions = _dimensions * glm::vec2(width / maxSize, height / maxSize);

        // FIXME - face and surfaceNormal not being set
        return findRayRectangleIntersection(origin, direction, getRotation(), getPosition(), dimensions, distance);
    }

    return false;
}

Image3DOverlay* Image3DOverlay::createClone() const {
    return new Image3DOverlay(this);
}
