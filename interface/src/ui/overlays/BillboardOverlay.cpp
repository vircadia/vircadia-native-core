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

#include "Application.h"

#include "BillboardOverlay.h"
#include <PlaneShape.h>

BillboardOverlay::BillboardOverlay() :
    _fromImage(),
    _scale(1.0f),
    _isFacingAvatar(true)
{
      _isLoaded = false;
}

BillboardOverlay::BillboardOverlay(const BillboardOverlay* billboardOverlay) :
    Base3DOverlay(billboardOverlay),
    _url(billboardOverlay->_url),
    _texture(billboardOverlay->_texture),
    _fromImage(billboardOverlay->_fromImage),
    _scale(billboardOverlay->_scale),
    _isFacingAvatar(billboardOverlay->_isFacingAvatar)
{
}

void BillboardOverlay::render(RenderArgs* args) {
    if (!_isLoaded) {
        _isLoaded = true;
        _texture = DependencyManager::get<TextureCache>()->getTexture(_url);
    }

    if (!_visible || !_texture->isLoaded()) {
        return;
    }

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    glBindTexture(GL_TEXTURE_2D, _texture->getID());

    glPushMatrix(); {
        glTranslatef(_position.x, _position.y, _position.z);
        glm::quat rotation;
        if (_isFacingAvatar) {
            // rotate about vertical to face the camera
            rotation = Application::getInstance()->getCamera()->getRotation();
            rotation *= glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
        } else {
            rotation = getRotation();
        }
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glScalef(_scale, _scale, _scale);

        const float MAX_COLOR = 255.0f;
        xColor color = getColor();
        float alpha = getAlpha();

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

        DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight,
                                                            glm::vec4(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha));

    } glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void BillboardOverlay::setProperties(const QScriptValue &properties) {
    Base3DOverlay::setProperties(properties);

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

    QScriptValue scaleValue = properties.property("scale");
    if (scaleValue.isValid()) {
        _scale = scaleValue.toVariant().toFloat();
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
    if (property == "scale") {
        return _scale;
    }
    if (property == "isFacingAvatar") {
        return _isFacingAvatar;
    }

    return Base3DOverlay::getProperty(property);
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

    if (_texture) {
        // This is taken from Planar3DOverlay.  Although it probably makes more sense for this
        // overlay type to extend from Planar3DOverlay, the interfaces are different enough (scale vs. dimensions)
        // that it would break existing scripts and would require other changes to make up for unique
        // features like `isFacingAvatar`.

        RayIntersectionInfo rayInfo;
        rayInfo._rayStart = origin;
        rayInfo._rayDirection = direction;
        rayInfo._rayLength = std::numeric_limits<float>::max();

        bool isNull = _fromImage.isNull();
        float width = isNull ? _texture->getWidth() : _fromImage.width();
        float height = isNull ? _texture->getHeight() : _fromImage.height();

        PlaneShape plane;

        const glm::vec3 UNROTATED_NORMAL(0.0f, 0.0f, -1.0f);
        glm::quat rotation;
        if (_isFacingAvatar) {
            // rotate about vertical to face the camera
            rotation = Application::getInstance()->getCamera()->getRotation();
            rotation *= glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
        } else {
            rotation = _rotation;
        }
        glm::vec3 normal = rotation * UNROTATED_NORMAL;
        plane.setNormal(normal);
        plane.setPoint(_position); // the position is definitely a point on our plane

        bool intersects = plane.findRayIntersection(rayInfo);

        if (intersects) {
            distance = rayInfo._hitDistance;

            glm::vec3 hitPosition = origin + (distance * direction);
            glm::vec3 localHitPosition = glm::inverse(rotation) * (hitPosition - _position);

            // Produce the dimensions of the billboard based on the image's aspect ratio and the overlay's scale.
            float maxSize = glm::max(width, height);
            glm::vec2 halfDimensions = 0.5f * _scale * glm::vec2(width / maxSize, height / maxSize);

            intersects = -halfDimensions.x <= localHitPosition.x && localHitPosition.x <= halfDimensions.x
                && -halfDimensions.y <= localHitPosition.y && localHitPosition.y <= halfDimensions.y;
        }
        return intersects;
    }
    return false;
}

BillboardOverlay* BillboardOverlay::createClone() const {
    return new BillboardOverlay(this);
}
