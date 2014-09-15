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
    
    // pass along the diffuse color
    gl_FrontColor = gl_FrontMaterial.diffuse;
    
    // and the texture coordinates
    gl_TexCoord[0] = gl_MultiTexCoord0;
    
    // use standard pipeline transform
    gl_Position = ftransform();
}
