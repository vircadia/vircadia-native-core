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

#include "AbstractViewStateInterface.h"

QString const Image3DOverlay::TYPE = "image3d";

Image3DOverlay::Image3DOverlay() {
    _isLoaded = false;
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Image3DOverlay::Image3DOverlay(const Image3DOverlay* image3DOverlay) :
    Billboard3DOverlay(image3DOverlay),
    _url(image3DOverlay->_url),
    _texture(image3DOverlay->_texture),
    _emissive(image3DOverlay->_emissive),
    _fromImage(image3DOverlay->_fromImage)
{
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Image3DOverlay::~Image3DOverlay() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

void Image3DOverlay::update(float deltatime) {
    if (!_isLoaded) {
        _isLoaded = true;
        _texture = DependencyManager::get<TextureCache>()->getTexture(_url);
        _textureIsLoaded = false;
    }
    Parent::update(deltatime);
}

void Image3DOverlay::render(RenderArgs* args) {
    if (!_renderVisible || !getParentVisible() || !_texture || !_texture->isLoaded()) {
        return;
    }

    // Once the texture has loaded, check if we need to update the render item because of transparency
    if (!_textureIsLoaded && _texture && _texture->getGPUTexture()) {
        _textureIsLoaded = true;
        bool prevAlphaTexture = _alphaTexture;
        _alphaTexture = _texture->getGPUTexture()->getUsage().isAlpha();
        if (_alphaTexture != prevAlphaTexture) {
            auto itemID = getRenderItemID();
            if (render::Item::isValidID(itemID)) {
                render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
                render::Transaction transaction;
                transaction.updateItem(itemID);
                scene->enqueueTransaction(transaction);
            }
        }
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
    float x = _keepAspectRatio ? fromImage.width() / (2.0f * maxSize) : 0.5f;
    float y = _keepAspectRatio ? -fromImage.height() / (2.0f * maxSize) : -0.5f;

    glm::vec2 topLeft(-x, -y);
    glm::vec2 bottomRight(x, y);
    glm::vec2 texCoordTopLeft((fromImage.x() + 0.5f) / imageWidth, (fromImage.y() + 0.5f) / imageHeight);
    glm::vec2 texCoordBottomRight((fromImage.x() + fromImage.width() - 0.5f) / imageWidth,
                                  (fromImage.y() + fromImage.height() - 0.5f) / imageHeight);

    float alpha = getAlpha();
    glm::u8vec3 color = getColor();
    glm::vec4 imageColor(toGlm(color), alpha);

    batch->setModelTransform(getRenderTransform());
    batch->setResourceTexture(0, _texture->getGPUTexture());

    DependencyManager::get<GeometryCache>()->renderQuad(
        *batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight,
        imageColor, _geometryId
    );

    batch->setResourceTexture(0, nullptr); // restore default white color after me
}

const render::ShapeKey Image3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withoutCullFace().withDepthBias();
    if (_emissive) {
        builder.withUnlit();
    }
    if (isTransparent()) {
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

    auto keepAspectRatioValue = properties["keepAspectRatio"];
    if (keepAspectRatioValue.isValid()) {
        _keepAspectRatio = keepAspectRatioValue.toBool();
    }

    auto emissiveValue = properties["emissive"];
    if (emissiveValue.isValid()) {
        _emissive = emissiveValue.toBool();
    }
}

/**jsdoc
 * These are the properties of an <code>image3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Image3DProperties
 *
 * @property {string} type=image3d - Has the value <code>"image3d"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and 
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {bool} keepAspectRatio=true - overlays will maintain the aspect ratio when the subImage is applied.
 *
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 *
 * @property {string} url - The URL of the PNG or JPG image to display.
 * @property {Rect} subImage - The portion of the image to display. Defaults to the full image.
 * @property {boolean} emissive - If <code>true</code>, the overlay is displayed at full brightness, otherwise it is rendered
 *     with scene lighting.
 */
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
    if (property == "keepAspectRatio") {
        return _keepAspectRatio;
    }

    return Billboard3DOverlay::getProperty(property);
}

void Image3DOverlay::setURL(const QString& url) {
    _url = url;
    _isLoaded = false;
}

bool Image3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                         float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precisionPicking) {
    if (_texture && _texture->isLoaded()) {
        Transform transform = getTransform();

        // Produce the dimensions of the overlay based on the image's aspect ratio and the overlay's scale.
        bool isNull = _fromImage.isNull();
        float width = isNull ? _texture->getWidth() : _fromImage.width();
        float height = isNull ? _texture->getHeight() : _fromImage.height();
        float maxSize = glm::max(width, height);
        glm::vec2 dimensions = _dimensions * glm::vec2(width / maxSize, height / maxSize);
        glm::quat rotation = transform.getRotation();

        if (findRayRectangleIntersection(origin, direction, rotation, transform.getTranslation(), dimensions, distance)) {
            glm::vec3 forward = rotation * Vectors::FRONT;
            if (glm::dot(forward, direction) > 0.0f) {
                face = MAX_Z_FACE;
                surfaceNormal = -forward;
            } else {
                face = MIN_Z_FACE;
                surfaceNormal = forward;
            }
            return true;
        }
    }

    return false;
}

bool Image3DOverlay::findParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                              float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal, bool precisionPicking) {
    if (_texture && _texture->isLoaded()) {
        Transform transform = getTransform();

        // Produce the dimensions of the overlay based on the image's aspect ratio and the overlay's scale.
        bool isNull = _fromImage.isNull();
        float width = isNull ? _texture->getWidth() : _fromImage.width();
        float height = isNull ? _texture->getHeight() : _fromImage.height();
        float maxSize = glm::max(width, height);
        glm::vec2 dimensions = _dimensions * glm::vec2(width / maxSize, height / maxSize);
        glm::quat rotation = transform.getRotation();
        glm::vec3 position = getWorldPosition();

        glm::quat inverseRot = glm::inverse(rotation);
        glm::vec3 localOrigin = inverseRot * (origin - position);
        glm::vec3 localVelocity = inverseRot * velocity;
        glm::vec3 localAcceleration = inverseRot * acceleration;

        if (findParabolaRectangleIntersection(localOrigin, localVelocity, localAcceleration, dimensions, parabolicDistance)) {
            float localIntersectionVelocityZ = localVelocity.z + localAcceleration.z * parabolicDistance;
            glm::vec3 forward = rotation * Vectors::FRONT;
            if (localIntersectionVelocityZ > 0.0f) {
                face = MIN_Z_FACE;
                surfaceNormal = forward;
            } else {
                face = MAX_Z_FACE;
                surfaceNormal = -forward;
            }
            return true;
        }
    }

    return false;
}

Image3DOverlay* Image3DOverlay::createClone() const {
    return new Image3DOverlay(this);
}

Transform Image3DOverlay::evalRenderTransform() {
    auto transform = Parent::evalRenderTransform();
    transform.postScale(glm::vec3(getDimensions(), 1.0f));
    return transform;
}
