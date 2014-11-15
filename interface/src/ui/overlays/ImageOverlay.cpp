//
//  ImageOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QPainter>
#include <QSvgRenderer>
#include <SharedUtil.h>

#include "ImageOverlay.h"

ImageOverlay::ImageOverlay() :
    _textureID(0),
    _renderImage(false),
    _textureBound(false),
    _wantClipFromImage(false)
{
    _isLoaded = false;
}

ImageOverlay::ImageOverlay(const ImageOverlay* imageOverlay) :
    Overlay2D(imageOverlay),
    _textureID(0),
    _textureBound(false),
    _wantClipFromImage(false),
    _renderImage(imageOverlay->_renderImage),
    _imageURL(imageOverlay->_imageURL),
    _textureImage(imageOverlay->_textureImage)
{
}

ImageOverlay::~ImageOverlay() {
    if (_parent && _textureID) {
        // do we need to call this?
        //_parent->deleteTexture(_textureID);
    }
}

// TODO: handle setting image multiple times, how do we manage releasing the bound texture?
void ImageOverlay::setImageURL(const QUrl& url) {
    _imageURL = url;
    _isLoaded = false;
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkReply* reply = networkAccessManager.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &ImageOverlay::replyFinished);
}

void ImageOverlay::replyFinished() {
    QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
    
    // replace our byte array with the downloaded data
    QByteArray rawData = reply->readAll();
    _textureImage.loadFromData(rawData);
    _renderImage = true;
    _isLoaded = true;
}

void ImageOverlay::render(RenderArgs* args) {
    if (!_visible || !_isLoaded) {
        return; // do nothing if we're not visible
    }
    if (_renderImage && !_textureBound) {
        _textureID = _parent->bindTexture(_textureImage);
        _textureBound = true;
    }

    if (_renderImage) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _textureID);
    }
    const float MAX_COLOR = 255.0f;
    xColor color = getColor();
    float alpha = getAlpha();
    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    float imageWidth = _textureImage.width();
    float imageHeight = _textureImage.height();
    
    QRect fromImage;
    if (_wantClipFromImage) {
        fromImage = _fromImage;
    } else {
        fromImage.setX(0);
        fromImage.setY(0);
        fromImage.setWidth(imageWidth);
        fromImage.setHeight(imageHeight);
    }
    float x = fromImage.x() / imageWidth;
    float y = fromImage.y() / imageHeight;
    float w = fromImage.width() / imageWidth; // ?? is this what we want? not sure
    float h = fromImage.height() / imageHeight;

    glBegin(GL_QUADS);
        if (_renderImage) {
            glTexCoord2f(x, 1.0f - y);
        }
        glVertex2f(_bounds.left(), _bounds.top());

        if (_renderImage) {
            glTexCoord2f(x + w, 1.0f - y);
        }
        glVertex2f(_bounds.right(), _bounds.top());

        if (_renderImage) {
            glTexCoord2f(x + w, 1.0f - (y + h));
        }
        glVertex2f(_bounds.right(), _bounds.bottom());

        if (_renderImage) {
            glTexCoord2f(x, 1.0f - (y + h));
        }
        glVertex2f(_bounds.left(), _bounds.bottom());
    glEnd();

    if (_renderImage) {
        glDisable(GL_TEXTURE_2D);
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
