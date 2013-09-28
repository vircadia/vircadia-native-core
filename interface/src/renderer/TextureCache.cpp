//
//  TextureCache.cpp
//  interface
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QOpenGLFramebufferObject>

#include <glm/gtc/random.hpp>

#include "Application.h"
#include "TextureCache.h"

TextureCache::TextureCache() : _permutationNormalTextureID(0),
    _primaryFramebufferObject(NULL), _secondaryFramebufferObject(NULL), _tertiaryFramebufferObject(NULL) {
}

TextureCache::~TextureCache() {
    if (_permutationNormalTextureID != 0) {
        glDeleteTextures(1, &_permutationNormalTextureID);
    }
    foreach (GLuint id, _fileTextureIDs) {
        glDeleteTextures(1, &id);
    }
    if (_primaryFramebufferObject != NULL) {
        delete _primaryFramebufferObject;
        glDeleteTextures(1, &_primaryDepthTextureID);
    }
    if (_secondaryFramebufferObject != NULL) {
        delete _secondaryFramebufferObject;
    }
    if (_tertiaryFramebufferObject != NULL) {
        delete _tertiaryFramebufferObject;
    }
}

GLuint TextureCache::getPermutationNormalTextureID() {
    if (_permutationNormalTextureID == 0) {
        glGenTextures(1, &_permutationNormalTextureID);
        glBindTexture(GL_TEXTURE_2D, _permutationNormalTextureID);
        
        // the first line consists of random permutation offsets
        unsigned char data[256 * 2 * 3];
        for (int i = 0; i < 256 * 3; i++) {
            data[i] = rand() % 256;
        }
        // the next, random unit normals
        for (int i = 256 * 3; i < 256 * 3 * 2; i += 3) {
            glm::vec3 randvec = glm::sphericalRand(1.0f);
            data[i] = ((randvec.x + 1.0f) / 2.0f) * 255.0f;
            data[i + 1] = ((randvec.y + 1.0f) / 2.0f) * 255.0f;
            data[i + 2] = ((randvec.z + 1.0f) / 2.0f) * 255.0f;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return _permutationNormalTextureID;
}

GLuint TextureCache::getFileTextureID(const QString& filename) {
    GLuint id = _fileTextureIDs.value(filename);
    if (id == 0) {
        switchToResourcesParentIfRequired();
        QImage image = QImage(filename).convertToFormat(QImage::Format_ARGB32);
    
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 1,
            GL_BGRA, GL_UNSIGNED_BYTE, image.constBits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        _fileTextureIDs.insert(filename, id);
    }
    return id;
}

QOpenGLFramebufferObject* TextureCache::getPrimaryFramebufferObject() {
    if (_primaryFramebufferObject == NULL) {
        _primaryFramebufferObject = createFramebufferObject();
        
        glGenTextures(1, &_primaryDepthTextureID);
        glBindTexture(GL_TEXTURE_2D, _primaryDepthTextureID);
        QSize size = Application::getInstance()->getGLWidget()->size();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.width(), size.height(),
            0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        _primaryFramebufferObject->bind();
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _primaryDepthTextureID, 0);
        _primaryFramebufferObject->release();
    }
    return _primaryFramebufferObject;
}

GLuint TextureCache::getPrimaryDepthTextureID() {
    // ensure that the primary framebuffer object is initialized before returning the depth texture id
    getPrimaryFramebufferObject();
    return _primaryDepthTextureID;
}

QOpenGLFramebufferObject* TextureCache::getSecondaryFramebufferObject() {
    if (_secondaryFramebufferObject == NULL) {
        _secondaryFramebufferObject = createFramebufferObject();
    }
    return _secondaryFramebufferObject;
}

QOpenGLFramebufferObject* TextureCache::getTertiaryFramebufferObject() {
    if (_tertiaryFramebufferObject == NULL) {
        _tertiaryFramebufferObject = createFramebufferObject();
    }
    return _tertiaryFramebufferObject;
}

bool TextureCache::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Resize) {
        QSize size = static_cast<QResizeEvent*>(event)->size();
        if (_primaryFramebufferObject != NULL && _primaryFramebufferObject->size() != size) {
            delete _primaryFramebufferObject;
            _primaryFramebufferObject = NULL;
            glDeleteTextures(1, &_primaryDepthTextureID);
        }
        if (_secondaryFramebufferObject != NULL && _secondaryFramebufferObject->size() != size) {
            delete _secondaryFramebufferObject;
            _secondaryFramebufferObject = NULL;
        }
        if (_tertiaryFramebufferObject != NULL && _tertiaryFramebufferObject->size() != size) {
            delete _tertiaryFramebufferObject;
            _tertiaryFramebufferObject = NULL;
        }
    }
    return false;
}

QOpenGLFramebufferObject* TextureCache::createFramebufferObject() {
    QOpenGLFramebufferObject* fbo = new QOpenGLFramebufferObject(Application::getInstance()->getGLWidget()->size());
    Application::getInstance()->getGLWidget()->installEventFilter(this);
    
    glBindTexture(GL_TEXTURE_2D, fbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return fbo;
}

Texture::Texture() {
    glGenTextures(1, &_id);
}

Texture::~Texture() {
    glDeleteTextures(1, &_id);
}

DilatedTextureCache::DilatedTextureCache(const QString& filename, int innerRadius, int outerRadius) :
    _innerRadius(innerRadius),
    _outerRadius(outerRadius)
{
    switchToResourcesParentIfRequired();
    _image = QImage(filename).convertToFormat(QImage::Format_ARGB32);
}

QSharedPointer<Texture> DilatedTextureCache::getTexture(float dilation) {
    QSharedPointer<Texture> texture = _textures.value(dilation);
    if (texture.isNull()) {
        texture = QSharedPointer<Texture>(new Texture());
        
        QImage dilatedImage = _image;
        QPainter painter;
        painter.begin(&dilatedImage);
        QPainterPath path;
        qreal radius = glm::mix(_innerRadius, _outerRadius, dilation);
        path.addEllipse(QPointF(_image.width() / 2.0, _image.height() / 2.0), radius, radius);
        painter.fillPath(path, Qt::black);
        painter.end();
        
        glBindTexture(GL_TEXTURE_2D, texture->getID());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dilatedImage.width(), dilatedImage.height(), 1,
            GL_BGRA, GL_UNSIGNED_BYTE, dilatedImage.constBits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        _textures.insert(dilation, texture);
    }
    return texture;
}
