#version 120

//
//  model_lightmap_normal_map.vert
//  vertex shader
//
//  Created by Sam Gateau on 11/21/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const int MAX_TEXCOORDS = 2;

uniform mat4 texcoordMatrices[MAX_TEXCOORDS];

// the tangent vector
attribute vec3 tangent;

attribute vec2 texcoord1;

// the interpolated normal
varying vec4 interpolatedNormal;

// the interpolated tangent
varying vec4 interpolatedTangent;

// the interpolated texcoord1
varying vec2 interpolatedTexcoord1;

void main(void) {
    // transform and store the normal and tangent for interpolation
    interpolatedNormal = gl_ModelViewMatrix * vec4(gl_Normal, 0.0);
    interpolatedTangent = gl_ModelViewMatrix * vec4(tangent, 0.0);
    
    // pass along the diffuse color
    gl_FrontColor = gl_Color * gl_FrontMaterial.diffuse;
    
    // and the texture coordinates
    gl_TexCoord[0] = texcoordMatrices[0] * vec4(gl_MultiTexCoord0.xy, 0.0, 1.0);
    interpolatedTexcoord1 = vec2(texcoordMatrices[1] * vec4(texcoord1.xy, 0.0, 1.0)).xy;
   
    // use standard pipeline transform
    gl_Position = ftransform();
}
