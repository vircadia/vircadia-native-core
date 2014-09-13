#version 120

//
//  model.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 10/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the interpolated normal
varying vec4 normal;

void main(void) {
    // transform and store the normal for interpolation
    normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    
    // pass along the vertex color
    gl_FrontColor = gl_Color;
    
    // and the texture coordinates
    gl_TexCoord[0] = gl_MultiTexCoord0;
    
    // use standard pipeline transform
    gl_Position = ftransform();
}

