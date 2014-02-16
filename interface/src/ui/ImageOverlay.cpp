//
//  ImageOverlay.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//


#include "ImageOverlay.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QGLWidget>
#include <SharedUtil.h>

ImageOverlay::ImageOverlay() :
    _parent(NULL),
    _textureID(0),
    _alpha(DEFAULT_ALPHA),
    _backgroundColor(DEFAULT_BACKGROUND_COLOR),
    _visible(true),
    _renderImage(false),
    _textureBound(false),
    _wantClipFromImage(false)
{
}

void ImageOverlay::init(QGLWidget* parent) {
    qDebug() << "ImageOverlay::init() parent=" << parent;
    _parent = parent;
}


ImageOverlay::~ImageOverlay() {
    if (_parent && _textureID) {
        // do we need to call this?
        //_parent->deleteTexture(_textureID);
    }
}

// TODO: handle setting image multiple times, how do we manage releasing the bound texture?
void ImageOverlay::setImageURL(const QUrl& url) {
    // TODO: are we creating too many QNetworkAccessManager() when multiple calls to setImageURL are made?
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    manager->get(QNetworkRequest(url));
}

void ImageOverlay::replyFinished(QNetworkReply* reply) {

    // replace our byte array with the downloaded data
    QByteArray rawData = reply->readAll();
    _textureImage.loadFromData(rawData);
    _renderImage = true;
    
}

void ImageOverlay::render() {
    if (!_visible) {
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
    const float MAX_COLOR = 255;
    glColor4f(_backgroundColor.red / MAX_COLOR, _backgroundColor.green / MAX_COLOR, _backgroundColor.blue / MAX_COLOR, _alpha);

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

// TODO: handle only setting the included values...
QScriptValue ImageOverlay::getProperties() {
    return QScriptValue();
}

// TODO: handle only setting the included values...
void ImageOverlay::setProperties(const QScriptValue& properties) {
    //qDebug() << "ImageOverlay::setProperties()... properties=" << &properties;
    QScriptValue bounds = properties.property("bounds");
    if (bounds.isValid()) {
        QRect boundsRect;
        boundsRect.setX(bounds.property("x").toVariant().toInt());
        boundsRect.setY(bounds.property("y").toVariant().toInt());
        boundsRect.setWidth(bounds.property("width").toVariant().toInt());
        boundsRect.setHeight(bounds.property("height").toVariant().toInt());
        setBounds(boundsRect);
    } else {
        QRect oldBounds = getBounds();
        QRect newBounds = oldBounds;
        
        if (properties.property("x").isValid()) {
            newBounds.setX(properties.property("x").toVariant().toInt());
        } else {
            newBounds.setX(oldBounds.x());
        }
        if (properties.property("y").isValid()) {
            newBounds.setY(properties.property("y").toVariant().toInt());
        } else {
            newBounds.setY(oldBounds.y());
        }
        if (properties.property("width").isValid()) {
            newBounds.setWidth(properties.property("width").toVariant().toInt());
        } else {
            newBounds.setWidth(oldBounds.width());
        }
        if (properties.property("height").isValid()) {
            newBounds.setHeight(properties.property("height").toVariant().toInt());
        } else {
            newBounds.setHeight(oldBounds.height());
        }
        setBounds(newBounds);
        //qDebug() << "set bounds to " << getBounds();
    }
    QScriptValue subImageBounds = properties.property("subImage");
    if (subImageBounds.isValid()) {
        QRect subImageRect = _fromImage;
        if (subImageBounds.property("x").isValid()) {
            subImageRect.setX(subImageBounds.property("x").toVariant().toInt());
        }
        if (subImageBounds.property("y").isValid()) {
            subImageRect.setY(subImageBounds.property("y").toVariant().toInt());
        }
        if (subImageBounds.property("width").isValid()) {
            subImageRect.setWidth(subImageBounds.property("width").toVariant().toInt());
        }
        if (subImageBounds.property("height").isValid()) {
            subImageRect.setHeight(subImageBounds.property("height").toVariant().toInt());
        }
        setClipFromSource(subImageRect);
    }    

    QScriptValue imageURL = properties.property("imageURL");
    if (imageURL.isValid()) {
        setImageURL(imageURL.toVariant().toString());
    }

    QScriptValue color = properties.property("backgroundColor");
    if (color.isValid()) {
        QScriptValue red = color.property("red");
        QScriptValue green = color.property("green");
        QScriptValue blue = color.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            _backgroundColor.red = red.toVariant().toInt();
            _backgroundColor.green = green.toVariant().toInt();
            _backgroundColor.blue = blue.toVariant().toInt();
        }
    }

    if (properties.property("alpha").isValid()) {
        setAlpha(properties.property("alpha").toVariant().toFloat());
    }

    if (properties.property("visible").isValid()) {
        setVisible(properties.property("visible").toVariant().toBool());
        qDebug() << "setting visible to " << getVisible();
    }
}


