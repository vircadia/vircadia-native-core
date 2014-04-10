#version 120

//
//  diffuse.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the texture containing the original color
uniform sampler2D originalTexture;

// the texture containing the diffused color
uniform sampler2D diffusedTexture;

// the scale of diffusion
uniform vec2 diffusionScale;

void main(void) {
    vec2 minExtents = gl_TexCoord[0].st + diffusionScale * vec2(-1.5, -1.5);
    vec2 maxExtents = gl_TexCoord[0].st + diffusionScale * vec2(1.5, 1.5);
    gl_FragColor = (texture2D(diffusedTexture, minExtents) +
        texture2D(diffusedTexture, vec2(maxExtents.s, minExtents.t)) + 
        texture2D(diffusedTexture, vec2(minExtents.s, maxExtents.t)) +
        texture2D(diffusedTexture, maxExtents)) * 0.235 + texture2D(originalTexture, gl_TexCoord[0].st) * 0.1;
}
