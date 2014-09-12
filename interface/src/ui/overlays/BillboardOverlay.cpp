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
  _isFacingAvatar(true) {
      _isLoaded = false;
}

void BillboardOverlay::render() {
    if (!_visible || !_isLoaded) {
        return;
    }
    
    if (!_billboard.isEmpty()) {
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
        if (_isFacingAvatar) {
            // rotate about vertical to face the camera
            glm::quat rotation = Application::getInstance()->getCamera()->getRotation();
            rotation *= glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 axis = glm::axis(rotation);
            glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        } else {
            glm::vec3 axis = glm::axis(_rotation);
            glRotatef(glm::degrees(glm::angle(_rotation)), axis.x, axis.y, axis.z);
        }
        glScalef(_scale, _scale, _scale);
        
        if (_billboardTexture) {
            float maxSize = glm::max(_fromImage.width(), _fromImage.height());
            float x = _fromImage.width() / (2.0f * maxSize);
            float y = -_fromImage.height() / (2.0f * maxSize);
            
            glColor3f(1.0f, 1.0f, 1.0f);
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
        _url = urlValue.toVariant().toString();
        
        setBillboardURL(_url);
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
    
    QScriptValue rotationValue = properties.property("rotation");
    if (rotationValue.isValid()) {
        QScriptValue x = rotationValue.property("x");
        QScriptValue y = rotationValue.property("y");
        QScriptValue z = rotationValue.property("z");
        QScriptValue w = rotationValue.property("w");
        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            _rotation.x = x.toVariant().toFloat();
            _rotation.y = y.toVariant().toFloat();
            _rotation.z = z.toVariant().toFloat();
            _rotation.w = w.toVariant().toFloat();
        }
    }
    
    QScriptValue isFacingAvatarValue = properties.property("isFacingAvatar");
    if (isFacingAvatarValue.isValid()) {
        _isFacingAvatar = isFacingAvatarValue.toVariant().toBool();
    }
}

void BillboardOverlay::setBillboardURL(const QUrl url) {
    _isLoaded = false;
    QNetworkReply* reply = NetworkAccessManager::getInstance().get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &BillboardOverlay::replyFinished);
}

void BillboardOverlay::replyFinished() {
    // replace our byte array with the downloaded data
    QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
    _billboard = reply->readAll();
    _isLoaded = true;
}
