//
//  RenderUtil.cpp
//  interface
//
//  Created by Andrzej Kapolka on 8/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "InterfaceConfig.h"
#include "RenderUtil.h"

void renderFullscreenQuad(float sMin, float sMax) {
    glBegin(GL_QUADS);
        glTexCoord2f(sMin, 0.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(sMax, 0.0f);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(sMax, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(sMin, 1.0f);
        glVertex2f(-1.0f, 1.0f);
    glEnd();
}
