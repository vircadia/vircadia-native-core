//
//  TextureCache.cpp
//  interface
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QGLWidget>
#include <QOpenGLFramebufferObject>

#include <glm/gtc/random.hpp>

#include "Application.h"
#include "TextureCache.h"

TextureCache::TextureCache() : _permutationNormalTextureID(0),
    _primaryFramebufferObject(NULL), _secondaryFramebufferObject(NULL) {
}

TextureCache::~TextureCache() {
    if (_permutationNormalTextureID != 0) {
        glDeleteTextures(1, &_permutationNormalTextureID);
    }
    if (_primaryFramebufferObject != NULL) {
        delete _primaryFramebufferObject;
    }
    if (_secondaryFramebufferObject != NULL) {
        delete _secondaryFramebufferObject;
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

QOpenGLFramebufferObject* TextureCache::getPrimaryFramebufferObject() {
    if (_primaryFramebufferObject == NULL) {
        _primaryFramebufferObject = new QOpenGLFramebufferObject(Application::getInstance()->getGLWidget()->size(),
            QOpenGLFramebufferObject::Depth);
        Application::getInstance()->getGLWidget()->installEventFilter(this);
    }
    return _primaryFramebufferObject;
}

QOpenGLFramebufferObject* TextureCache::getSecondaryFramebufferObject() {
    if (_secondaryFramebufferObject == NULL) {
        _secondaryFramebufferObject = new QOpenGLFramebufferObject(Application::getInstance()->getGLWidget()->size());
        Application::getInstance()->getGLWidget()->installEventFilter(this);
    }
    return _secondaryFramebufferObject;
}

bool TextureCache::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Resize) {
        QSize size = static_cast<QResizeEvent*>(event)->size();
        if (_primaryFramebufferObject != NULL && _primaryFramebufferObject->size() != size) {
            delete _primaryFramebufferObject;
            _primaryFramebufferObject = NULL;
        }
        if (_secondaryFramebufferObject != NULL && _secondaryFramebufferObject->size() != size) {
            delete _secondaryFramebufferObject;
            _secondaryFramebufferObject = NULL;
        }
    }
    return false;
}
