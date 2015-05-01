//
//  SimpleDisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "DisplayPlugin.h"
#include <QOpenGLContext>
#include <GLMHelpers.h>
#include <RenderUtil.h>

template <typename T>
class SimpleDisplayPlugin : public DisplayPlugin {
public:
    virtual void render(int finalSceneTexture) {
        makeCurrent();

        glDisable(GL_LIGHTING);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, finalSceneTexture);
        renderFullscreenQuad();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glEnable(GL_LIGHTING);

        swapBuffers();
        doneCurrent();
    }

    virtual glm::ivec2 getUiMousePosition() const {
        return getTrueMousePosition();
    }

    virtual glm::ivec2 getTrueMousePosition() const {
        return toGlm(_window->mapFromGlobal(QCursor::pos()));
    }

protected:
    T * _window;
};
