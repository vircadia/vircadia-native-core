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
    _renderImage(false),
    _textureBound(false)
{
}

void ImageOverlay::init(QGLWidget* parent) {
    qDebug() << "ImageOverlay::init() parent=" << parent;
    _parent = parent;
    
    /*
    qDebug() << "ImageOverlay::init()... url=" << url;
    _bounds = drawAt;
    _fromImage = fromImage;
    setImageURL(url);
    */
}


ImageOverlay::~ImageOverlay() {
    if (_parent && _textureID) {
        // do we need to call this?
        //_parent->deleteTexture(_textureID);
    }
}

void ImageOverlay::setImageURL(const QUrl& url) {
    // TODO: are we creating too many QNetworkAccessManager() when multiple calls to setImageURL are made?
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    manager->get(QNetworkRequest(url));
}

void ImageOverlay::replyFinished(QNetworkReply* reply) {
    qDebug() << "ImageOverlay::replyFinished() reply=" << reply;
    // replace our byte array with the downloaded data
    QByteArray rawData = reply->readAll();
    _textureImage.loadFromData(rawData);
    _renderImage = true;
    
    // TODO: handle setting image multiple times, how do we manage releasing the bound texture
    qDebug() << "ImageOverlay::replyFinished() about to call _parent->bindTexture(_textureImage)... _parent" << _parent; 


    qDebug() << "ImageOverlay::replyFinished _textureID=" << _textureID 
        << "_textureImage.width()=" << _textureImage.width()
        << "_textureImage.height()=" << _textureImage.height();
}

void ImageOverlay::render() {
    //qDebug() << "ImageOverlay::render _textureID=" << _textureID << "_bounds=" << _bounds;
    
    if (_renderImage && !_textureBound) {
        _textureID = _parent->bindTexture(_textureImage);
        _textureBound = true;
    }

    if (_renderImage) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _textureID);
    }
    const float MAX_COLOR = 255;
    glColor4f((_backgroundColor.red / MAX_COLOR), (_backgroundColor.green / MAX_COLOR), (_backgroundColor.blue / MAX_COLOR), _alpha);

    float imageWidth = _textureImage.width();
    float imageHeight = _textureImage.height();
    float x = _fromImage.x() / imageWidth;
    float y = _fromImage.y() / imageHeight;
    float w = _fromImage.width() / imageWidth; // ?? is this what we want? not sure
    float h = _fromImage.height() / imageHeight;

    //qDebug() << "ImageOverlay::render x=" << x << "y=" << y << "w="<<w << "h="<<h << "(1.0f - y)=" << (1.0f - y);
    
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
    QScriptValue bounds = properties.property("bounds");
    if (bounds.isValid()) {
        QRect boundsRect;
        boundsRect.setX(bounds.property("x").toVariant().toInt());
        boundsRect.setY(bounds.property("y").toVariant().toInt());
        boundsRect.setWidth(bounds.property("width").toVariant().toInt());
        boundsRect.setHeight(bounds.property("height").toVariant().toInt());
        setBounds(boundsRect);
    } else {
        setX(properties.property("x").toVariant().toInt());
        setY(properties.property("y").toVariant().toInt());
        setWidth(properties.property("width").toVariant().toInt());
        setHeight(properties.property("height").toVariant().toInt());
    }
    QScriptValue subImageBounds = properties.property("subImage");
    if (subImageBounds.isValid()) {
        QRect subImageRect;
        subImageRect.setX(subImageBounds.property("x").toVariant().toInt());
        subImageRect.setY(subImageBounds.property("y").toVariant().toInt());
        subImageRect.setWidth(subImageBounds.property("width").toVariant().toInt());
        subImageRect.setHeight(subImageBounds.property("height").toVariant().toInt());
        setClipFromSource(subImageRect);
        qDebug() << "set subImage to " << getClipFromSource();
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

    setAlpha(properties.property("alpha").toVariant().toFloat());
}


