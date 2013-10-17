#version 120

//
//  blendface.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 10/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the interpolated normal
varying vec4 normal;

void main(void) {

    // transform and store the normal for interpolation
    normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    
    // pass along the texture coordinate
    gl_TexCoord[0] = gl_MultiTexCoord0;
    
    // use standard pipeline transform
    gl_Position = ftransform();
}
