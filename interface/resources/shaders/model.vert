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

const int MAX_TEXCOORDS = 2;

uniform mat4 texcoordMatrices[MAX_TEXCOORDS];

// the interpolated normal
varying vec4 normal;

void main(void) {
    // transform and store the normal for interpolation
    normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    
    // pass along the diffuse color
    gl_FrontColor = gl_Color * gl_FrontMaterial.diffuse;
    
    // and the texture coordinates
    gl_TexCoord[0] = texcoordMatrices[0] * vec4(gl_MultiTexCoord0.xy, 0.f, 1.f);
    
    // use standard pipeline transform
    gl_Position = ftransform();
}

