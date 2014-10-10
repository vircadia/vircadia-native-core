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

#include <NetworkAccessManager.h>

#include "Application.h"

#include "BillboardOverlay.h"

BillboardOverlay::BillboardOverlay()
: _fromImage(-1,-1,-1,-1),
  _scale(1.0f),
  _isFacingAvatar(true),
  _newTextureNeeded(true) {
      _isLoaded = false;
}

void BillboardOverlay::render() {
    if (!_visible || !_isLoaded) {
        return;
    }
    
    if (!_billboard.isEmpty()) {
        if (_newTextureNeeded && _billboardTexture) {
            _billboardTexture.reset();
        }
        if (!_billboardTexture) {
            QImage image = QImage::fromData(_billboard);
            if (image.format() != QImage::Format_ARGB32) {
                image = image.convertToFormat(QImage::Format_ARGB32);
            }
            _size = image.size();
            if (_fromImage.x() == -1) {
                _fromImage.setRect(0, 0, _size.width(), _size.height());
            }
            _billboardTexture.reset(new Texture());
            _newTextureNeeded = false;
            glBindTexture(GL_TEXTURE_2D, _billboardTexture->getID());
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _size.width(), _size.height(), 0,
                         GL_BGRA, GL_UNSIGNED_BYTE, image.constBits());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            
        } else {
            glBindTexture(GL_TEXTURE_2D, _billboardTexture->getID());
        }
    }
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    
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
        
        if (_billboardTexture) {
            float maxSize = glm::max(_fromImage.width(), _fromImage.height());
            float x = _fromImage.width() / (2.0f * maxSize);
            float y = -_fromImage.height() / (2.0f * maxSize);
            
            const float MAX_COLOR = 255.0f;
            xColor color = getColor();
            float alpha = getAlpha();
            glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);
            glBegin(GL_QUADS); {
                glTexCoord2f((float)_fromImage.x() / (float)_size.width(),
                             (float)_fromImage.y() / (float)_size.height());

                glVertex2f(-x, -y);
                glTexCoord2f(((float)_fromImage.x() + (float)_fromImage.width()) / (float)_size.width(),
                             (float)_fromImage.y() / (float)_size.height());
                glVertex2f(x, -y);
                glTexCoord2f(((float)_fromImage.x() + (float)_fromImage.width()) / (float)_size.width(),
                             ((float)_fromImage.y() + (float)_fromImage.height()) / _size.height());
                glVertex2f(x, y);
                glTexCoord2f((float)_fromImage.x() / (float)_size.width(),
                             ((float)_fromImage.y() + (float)_fromImage.height()) / (float)_size.height());
                glVertex2f(-x, y);
            } glEnd();
        }
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
    
    QScriptValue scaleValue = properties.property("scale");
    if (scaleValue.isValid()) {
        _scale = scaleValue.toVariant().toFloat();
    }
    
    QScriptValue isFacingAvatarValue = properties.property("isFacingAvatar");
    if (isFacingAvatarValue.isValid()) {
        _isFacingAvatar = isFacingAvatarValue.toVariant().toBool();
    }
}

void BillboardOverlay::setURL(const QString& url) {
    setBillboardURL(url);
}

void BillboardOverlay::setBillboardURL(const QString& url) {
    _url = url;
    QUrl actualURL = url;

    _isLoaded = false;

    // clear the billboard if previously set
    _billboard.clear();
    _newTextureNeeded = true;

    QNetworkReply* reply = NetworkAccessManager::getInstance().get(QNetworkRequest(actualURL));
    connect(reply, &QNetworkReply::finished, this, &BillboardOverlay::replyFinished);
}

void BillboardOverlay::replyFinished() {
    // replace our byte array with the downloaded data
    QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
    _billboard = reply->readAll();
    _isLoaded = true;
}

bool BillboardOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face) const {

    if (_billboardTexture) {
        float maxSize = glm::max(_fromImage.width(), _fromImage.height());
        float x = _fromImage.width() / (2.0f * maxSize);
        float y = -_fromImage.height() / (2.0f * maxSize);
        float maxDimension = glm::max(x,y);
        float scaledDimension = maxDimension * _scale;
        glm::vec3 corner = getCenter() - glm::vec3(scaledDimension, scaledDimension, scaledDimension) ;
        AACube myCube(corner, scaledDimension * 2.0f);
        return myCube.findRayIntersection(origin, direction, distance, face);
    }
    return false;
}

