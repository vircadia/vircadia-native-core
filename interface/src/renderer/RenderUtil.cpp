//
//  RenderUtil.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 8/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
