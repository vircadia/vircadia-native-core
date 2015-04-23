//
//  RenderUtil.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 8/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderUtil_h
#define hifi_RenderUtil_h

/// Renders a quad from (-1, -1, 0) to (1, 1, 0) with texture coordinates from (sMin, tMin) to (sMax, tMax).
void renderFullscreenQuad(float sMin = 0.0f, float sMax = 1.0f, float tMin = 0.0f, float tMax = 1.0f);

template <typename F, GLenum matrix>
void withMatrixPush(F f) {
    glMatrixMode(matrix);
    glPushMatrix();
    f();
    glPopMatrix();
}

template <typename F>
void withProjectionPush(F f) {
    withMatrixPush<GL_PROJECTION>(f);
}

template <typename F>
void withProjectionIdentity(F f) {
    withProjectionPush([&] {
        glLoadIdentity();
        f();
    });
}

template <typename F>
void withProjectionMatrix(GLfloat* matrix, F f) {
    withProjectionPush([&] {
        glLoadMatrixf(matrix);
        f();
    });
}

template <typename F>
void withModelviewPush(F f) {
    withMatrixPush<GL_MODELVIEW>(f);
}

template <typename F>
void withModelviewIdentity(F f) {
    withModelviewPush([&] {
        glLoadIdentity();
        f();
    });
}

template <typename F>
void withModelviewMatrix(GLfloat* matrix, F f) {
    withModelviewPush([&] {
        glLoadMatrixf(matrix);
        f();
    });
}

#endif // hifi_RenderUtil_h
