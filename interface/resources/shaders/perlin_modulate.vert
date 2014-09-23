#version 120

//
//  perlin_modulate.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 5/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the position in model space
varying vec3 position;

// the interpolated normal
varying vec4 normal;

void main(void) {
    position = gl_Vertex.xyz;
    normal = vec4(gl_NormalMatrix * gl_Normal, 0.0);
    gl_FrontColor = gl_Color;
    gl_Position = ftransform();
}
