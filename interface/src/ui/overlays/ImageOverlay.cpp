//
//  ImageOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ImageOverlay.h"

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>
#include <RegisteredMetaTypes.h>

ImageOverlay::ImageOverlay() :
    _imageURL(),
    _renderImage(false),
    _wantClipFromImage(false)
{
}

ImageOverlay::ImageOverlay(const ImageOverlay* imageOverlay) :
    Overlay2D(imageOverlay),
    _imageURL(imageOverlay->_imageURL),
    _textureImage(imageOverlay->_textureImage),
    _texture(imageOverlay->_texture),
    _fromImage(imageOverlay->_fromImage),
    _renderImage(imageOverlay->_renderImage),
    _wantClipFromImage(imageOverlay->_wantClipFromImage)
{
}

// TODO: handle setting image multiple times, how do we manage releasing the bound texture?
void ImageOverlay::setImageURL(const QUrl& url) {
    _imageURL = url;
    if (url.isEmpty()) {
        _isLoaded = true;
        _renderImage = false;
        _texture.clear();
    } else {
        _isLoaded = false;
        _renderImage = true;
    }
}

void ImageOverlay::render(RenderArgs* args) {
    if (!_isLoaded && _renderImage) {
        _isLoaded = true;
        _texture = DependencyManager::get<TextureCache>()->getTexture(_imageURL);
    }
    // If we are not visible or loaded, return.  If we are trying to render an
    // image but the texture hasn't loaded, return.
    if (!_visible || !_isLoaded || (_renderImage && !_texture->isLoaded())) {
        return;
    }

    auto geometryCache = DependencyManager::get<GeometryCache>();
    gpu::Batch& batch = *args->_batch;
    geometryCache->useSimpleDrawPipeline(batch);
    if (_renderImage) {
        batch.setResourceTexture(0, _texture->getGPUTexture());
    } else {
        batch.setResourceTexture(0, args->_whiteTexture);
    }

    const float MAX_COLOR = 255.0f;
    xColor color = getColor();
    float alpha = getAlpha();
    glm::vec4 quadColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    int left = _bounds.left();
    int right = _bounds.right() + 1;
    int top = _bounds.top();
    int bottom = _bounds.bottom() + 1;

    glm::vec2 topLeft(left, top);
    glm::vec2 bottomRight(right, bottom);

    batch.setModelTransform(Transform());

    // if for some reason our image is not over 0 width or height, don't attempt to render the image
    if (_renderImage) {
        float imageWidth = _texture->getWidth();
        float imageHeight = _texture->getHeight();
        if (imageWidth > 0 && imageHeight > 0) {
            QRect fromImage;
            if (_wantClipFromImage) {
                float scaleX = imageWidth / _texture->getOriginalWidth();
                float scaleY = imageHeight / _texture->getOriginalHeight();

                fromImage.setX(scaleX * _fromImage.x());
                fromImage.setY(scaleY * _fromImage.y());
                fromImage.setWidth(scaleX * _fromImage.width());
                fromImage.setHeight(scaleY * _fromImage.height());
            }
            else {
                fromImage.setX(0);
                fromImage.setY(0);
                fromImage.setWidth(imageWidth);
                fromImage.setHeight(imageHeight);
            }

            float x = fromImage.x() / imageWidth;
            float y = fromImage.y() / imageHeight;
            float w = fromImage.width() / imageWidth; // ?? is this what we want? not sure
            float h = fromImage.height() / imageHeight;

            glm::vec2 texCoordTopLeft(x, y);
            glm::vec2 texCoordBottomRight(x + w, y + h);
            glm::vec4 texcoordRect(texCoordTopLeft, w, h);

            DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, quadColor);
        } else {
            DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, quadColor);
        }
    } else {
        DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, quadColor);
    }
}

void ImageOverlay::setProperties(const QScriptValue& properties) {
    Overlay2D::setProperties(properties);

    QScriptValue subImageBounds = properties.property("subImage");
    if (subImageBounds.isValid()) {
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

    QScriptValue imageURL = properties.property("imageURL");
    if (imageURL.isValid()) {
        setImageURL(imageURL.toVariant().toString());
    }
}

QScriptValue ImageOverlay::getProperty(const QString& property) {
    if (property == "subImage") {
        return qRectToScriptValue(_scriptEngine, _fromImage);
    }
    if (property == "imageURL") {
        return _imageURL.toString();
    }

    return Overlay2D::getProperty(property);
}
ImageOverlay* ImageOverlay::createClone() const {
    return new ImageOverlay(this);
}
