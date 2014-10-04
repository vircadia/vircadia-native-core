#version 120

//
//  metavoxel_voxel_splat.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 9/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the number of splats per pass
const int SPLAT_COUNT = 4;

// the splat textures
uniform sampler2D diffuseMaps[SPLAT_COUNT];

// the model space normal
varying vec3 normal;

// alpha values for the four splat textures
varying vec4 alphaValues;

void main(void) {
    // determine the cube face to use for texture coordinate generation
    vec3 absNormal = abs(normal);
    vec3 steps = step(absNormal.zzy, absNormal.xyx);
    vec2 parameters = mix(vec2(0.0, steps.y), vec2(steps.x, steps.x), steps.z);
    
    // blend the splat textures
    gl_FragColor = (texture2D(diffuseMaps[0], mix(gl_TexCoord[0].xy, gl_TexCoord[0].zw, parameters)) * alphaValues.x +
        texture2D(diffuseMaps[1], mix(gl_TexCoord[1].xy, gl_TexCoord[1].zw, parameters)) * alphaValues.y +
        texture2D(diffuseMaps[2], mix(gl_TexCoord[2].xy, gl_TexCoord[2].zw, parameters)) * alphaValues.z +
        texture2D(diffuseMaps[3], mix(gl_TexCoord[3].xy, gl_TexCoord[3].zw, parameters)) * alphaValues.w);
}
