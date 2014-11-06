#version 120

//
//  deferred_light_limited.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 9/19/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

void main(void) {
    gl_Position = ftransform();
    vec4 projected = gl_Position / gl_Position.w;
    gl_TexCoord[0] = vec4(dot(projected, gl_ObjectPlaneS[3]) * gl_Position.w,
        dot(projected, gl_ObjectPlaneT[3]) * gl_Position.w, 0.0, gl_Position.w);
}
