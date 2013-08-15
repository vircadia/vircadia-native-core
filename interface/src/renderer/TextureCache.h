//
//  TextureCache.h
//  interface
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__TextureCache__
#define __interface__TextureCache__

#include <QObject>

#include "InterfaceConfig.h"

class QOpenGLFramebufferObject;

class TextureCache : public QObject {
public:
    
    TextureCache();
    ~TextureCache();
    
    GLuint getPermutationNormalTextureID();

    QOpenGLFramebufferObject* getPrimaryFramebufferObject();
    QOpenGLFramebufferObject* getSecondaryFramebufferObject();
    QOpenGLFramebufferObject* getTertiaryFramebufferObject();
    
    virtual bool eventFilter(QObject* watched, QEvent* event);

private:
    
    GLuint _permutationNormalTextureID;
    
    QOpenGLFramebufferObject* _primaryFramebufferObject;
    QOpenGLFramebufferObject* _secondaryFramebufferObject;
    QOpenGLFramebufferObject* _tertiaryFramebufferObject;
};

#endif /* defined(__interface__TextureCache__) */
