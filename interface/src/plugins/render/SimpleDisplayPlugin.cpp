//
//  SimpleDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SimpleDisplayPlugin.h"
#include <QOpenGLContext>
#include <QCursor>

#include <RenderUtil.h>
#include "DependencyManager.h"
#include "TextureCache.h"
#include "gpu/GLBackend.h"


void SimpleGlDisplayPlugin::activate() {
    makeCurrent();
    doneCurrent();
}

void SimpleGlDisplayPlugin::display(GLuint sceneTexture, const glm::uvec2& sceneSize,
                                    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    makeCurrent();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    
    if (sceneTexture) {
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(+1, -1);
        glTexCoord2f(1, 1);
        glVertex2f(+1, +1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, +1);
        glEnd();
    }

    if (overlayTexture) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, overlayTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(+1, -1);
        glTexCoord2f(1, 1);
        glVertex2f(+1, +1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, +1);
        glEnd();
    }

    
    glDisable(GL_BLEND);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    Q_ASSERT(!glGetError());
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glFinish();
    doneCurrent();
}
