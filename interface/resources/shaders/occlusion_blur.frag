#version 120

//
//  occlusion_blur.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/16/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the original texture
uniform sampler2D originalTexture;

// the scale for the blur kernel
uniform vec2 blurScale;

void main(void) {
    vec2 minExtents = gl_TexCoord[0].st + blurScale * vec2(-0.5, -0.5);
    vec2 maxExtents = gl_TexCoord[0].st + blurScale * vec2(1.5, 1.5);
    gl_FragColor = (texture2D(originalTexture, minExtents) +
        texture2D(originalTexture, vec2(maxExtents.s, minExtents.t)) +
        texture2D(originalTexture, vec2(minExtents.s, maxExtents.t)) +
        texture2D(originalTexture, maxExtents)) * 0.25;
}
