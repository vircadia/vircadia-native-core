//
//  BillboardOverlay.cpp
//
//
//  Created by Clement on 7/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BillboardOverlay.h"

#include <QScriptValue>

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <gpu/Batch.h>

#include "Application.h"
#include "GeometryUtil.h"

BillboardOverlay::BillboardOverlay() {
      _isLoaded = false;
}

BillboardOverlay::BillboardOverlay(const BillboardOverlay* billboardOverlay) :
    Planar3DOverlay(billboardOverlay),
    _url(billboardOverlay->_url),
    _texture(billboardOverlay->_texture),
    _fromImage(billboardOverlay->_fromImage),
    _isFacingAvatar(billboardOverlay->_isFacingAvatar)
{
}

void BillboardOverlay::render(RenderArgs* args) {
    if (!_texture) {
        _isLoaded = true;
        _texture = DependencyManager::get<TextureCache>()->getTexture(_url);
    }

    if (!_visible || !_texture || !_texture->isLoaded()) {
        return;
    }

    glm::quat rotation;
    if (_isFacingAvatar) {
        // rotate about vertical to face the camera
        rotation = args->_viewFrustum->getOrientation();
        rotation *= glm::angleAxis(glm::pi<float>(), IDENTITY_UP);
        rotation *= getRotation();
    } else {
        rotation = getRotation();
    }

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

    auto batch = args->_batch;

    if (batch) {
        Transform transform = _transform;
        transform.postScale(glm::vec3(getDimensions(), 1.0f));
        
        batch->setModelTransform(transform);
        batch->setResourceTexture(0, _texture->getGPUTexture());
        
        DependencyManager::get<GeometryCache>()->renderQuad(*batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight,
                                                            glm::vec4(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha));
    
        batch->setResourceTexture(0, args->_whiteTexture); // restore default white color after me
    }
}

void BillboardOverlay::setProperties(const QScriptValue &properties) {
    Planar3DOverlay::setProperties(properties);

    QScriptValue urlValue = properties.property("url");
    if (urlValue.isValid()) {
        QString newURL = urlValue.toVariant().toString();
        if (newURL != _url) {
            setBillboardURL(newURL);
        }
    }

    QScriptValue subImageBounds = properties.property("subImage");
    if (subImageBounds.isValid()) {
        if (subImageBounds.isNull()) {
            _fromImage = QRect();
        } else {
            QRect oldSubImageRect = _fromImage;
            QRect subImageRect = _fromImage;
            if (subImageBounds.property("x").isValid()) {
                subImageRect.setX(subImageBounds.property("x").toVariant().toInt());
            } else {
                subImageRect.setX(oldSubImageRect.x());
            }
            if (subImageBounds.property("y").isValid()) {
                subImageRect.setY(subImageBounds.property("y").toVariant().toInt());
            } else {
                subImageRect.setY(oldSubImageRect.y());
            }
            if (subImageBounds.property("width").isValid()) {
                subImageRect.setWidth(subImageBounds.property("width").toVariant().toInt());
            } else {
                subImageRect.setWidth(oldSubImageRect.width());
            }
            if (subImageBounds.property("height").isValid()) {
                subImageRect.setHeight(subImageBounds.property("height").toVariant().toInt());
            } else {
                subImageRect.setHeight(oldSubImageRect.height());
            }
            setClipFromSource(subImageRect);
        }
    }

    QScriptValue isFacingAvatarValue = properties.property("isFacingAvatar");
    if (isFacingAvatarValue.isValid()) {
        _isFacingAvatar = isFacingAvatarValue.toVariant().toBool();
    }
}

QScriptValue BillboardOverlay::getProperty(const QString& property) {
    if (property == "url") {
        return _url;
    }
    if (property == "subImage") {
        return qRectToScriptValue(_scriptEngine, _fromImage);
    }
    if (property == "isFacingAvatar") {
        return _isFacingAvatar;
    }

    return Planar3DOverlay::getProperty(property);
}

void BillboardOverlay::setURL(const QString& url) {
    setBillboardURL(url);
}

void BillboardOverlay::setBillboardURL(const QString& url) {
    _url = url;
    _isLoaded = false;
}

bool BillboardOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face) {

    if (_texture && _texture->isLoaded()) {
        glm::quat rotation = getRotation();
        if (_isFacingAvatar) {
            // rotate about vertical to face the camera
            rotation = Application::getInstance()->getCamera()->getRotation();
            rotation *= glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Produce the dimensions of the billboard based on the image's aspect ratio and the overlay's scale.
        bool isNull = _fromImage.isNull();
        float width = isNull ? _texture->getWidth() : _fromImage.width();
        float height = isNull ? _texture->getHeight() : _fromImage.height();
        float maxSize = glm::max(width, height);
        glm::vec2 dimensions = _dimensions * glm::vec2(width / maxSize, height / maxSize);

        return findRayRectangleIntersection(origin, direction, rotation, getPosition(), dimensions, distance);
    }

    return false;
}

BillboardOverlay* BillboardOverlay::createClone() const {
    return new BillboardOverlay(this);
}
