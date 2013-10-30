#version 120

//
//  model.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 10/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the tangent vector
attribute vec3 tangent;

// the interpolated normal
varying vec4 interpolatedNormal;

// the interpolated tangent
varying vec4 interpolatedTangent;

void main(void) {

    // transform and store the normal and tangent for interpolation
    interpolatedNormal = gl_ModelViewMatrix * vec4(gl_Normal, 0.0);
    interpolatedTangent = gl_ModelViewMatrix * vec4(tangent, 0.0);
    
    // pass along the vertex color
    gl_FrontColor = gl_Color;
    
    // and the texture coordinates
    gl_TexCoord[0] = gl_MultiTexCoord0;
    
    // use standard pipeline transform
    gl_Position = ftransform();
}
