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
    vec3 steps = smoothstep(absNormal.zzy - vec3(0.05, 0.05, 0.05), absNormal.zzy + vec3(0.05, 0.05, 0.05), absNormal.xyx);
    
    // blend the splat textures
    vec4 base0 = texture2D(diffuseMaps[0], gl_TexCoord[0].xy);
    vec4 base1 = texture2D(diffuseMaps[1], gl_TexCoord[1].xy);
    vec4 base2 = texture2D(diffuseMaps[2], gl_TexCoord[2].xy);
    vec4 base3 = texture2D(diffuseMaps[3], gl_TexCoord[3].xy);
    gl_FragColor =
        mix(mix(base0, texture2D(diffuseMaps[0], gl_TexCoord[0].xw), steps.y),
            mix(base0, texture2D(diffuseMaps[0], gl_TexCoord[0].zw), steps.x), steps.z) * alphaValues.x +
        mix(mix(base1, texture2D(diffuseMaps[1], gl_TexCoord[1].xw), steps.y),
            mix(base1, texture2D(diffuseMaps[1], gl_TexCoord[1].zw), steps.x), steps.z) * alphaValues.y +
        mix(mix(base2, texture2D(diffuseMaps[2], gl_TexCoord[2].xw), steps.y),
            mix(base2, texture2D(diffuseMaps[2], gl_TexCoord[2].zw), steps.x), steps.z) * alphaValues.z +
        mix(mix(base3, texture2D(diffuseMaps[3], gl_TexCoord[3].xw), steps.y),
            mix(base3, texture2D(diffuseMaps[3], gl_TexCoord[3].zw), steps.x), steps.z) * alphaValues.w;
}
