//
//  RenderUtil.cpp
//  interface
//
//  Created by Andrzej Kapolka on 8/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "InterfaceConfig.h"
#include "RenderUtil.h"

void renderFullscreenQuad() {
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
    glEnd();
}
