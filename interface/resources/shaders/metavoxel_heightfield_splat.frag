#version 120

//
//  metavoxel_heightfield_splat.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const int SPLAT_COUNT = 4;

// the splat textures
uniform sampler2D diffuseMaps[SPLAT_COUNT];

// alpha values for the four splat textures
varying vec4 alphaValues;

void main(void) {
    // blend the splat textures
    gl_FragColor = gl_Color * (texture2D(diffuseMaps[0], gl_TexCoord[0].st) * alphaValues.x +
        texture2D(diffuseMaps[1], gl_TexCoord[0].st) * alphaValues.y +
        texture2D(diffuseMaps[2], gl_TexCoord[0].st) * alphaValues.z +
        texture2D(diffuseMaps[3], gl_TexCoord[0].st) * alphaValues.w);
}
