#version 120

//
//  voxel.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 9/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the interpolated normal
varying vec4 normal;

void main(void) {
    // store the normal for interpolation
    normal = vec4(gl_NormalMatrix * gl_Normal, 0.0);
    
    // use the standard position
    gl_Position = ftransform();
    
    // store the color directly
    gl_FrontColor = gl_Color;
}
