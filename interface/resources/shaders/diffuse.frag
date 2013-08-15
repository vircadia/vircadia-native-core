#version 120

//
//  diffuse.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the texture containing the original color
uniform sampler2D originalTexture;

// the texture containing the diffused color
uniform sampler2D diffusedTexture;

void main(void) {
    float ds = dFdx(gl_TexCoord[0].s);
    float dt = dFdy(gl_TexCoord[0].t);
    gl_FragColor = (texture2D(diffusedTexture, gl_TexCoord[0].st + vec2(-ds, -dt)) +
        texture2D(diffusedTexture, gl_TexCoord[0].st + vec2(0.0, -dt)) +
        texture2D(diffusedTexture, gl_TexCoord[0].st + vec2(ds, -dt)) +
        texture2D(diffusedTexture, gl_TexCoord[0].st + vec2(ds, 0.0)) +
        texture2D(diffusedTexture, gl_TexCoord[0].st + vec2(ds, dt)) +
        texture2D(diffusedTexture, gl_TexCoord[0].st + vec2(0.0, dt)) +
        texture2D(diffusedTexture, gl_TexCoord[0].st + vec2(-ds, dt)) +
        texture2D(diffusedTexture, gl_TexCoord[0].st + vec2(-ds, 0.0))) / 8.5 + texture2D(originalTexture, gl_TexCoord[0].st) * 0.1;
}
