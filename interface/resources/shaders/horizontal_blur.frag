#version 120

//
//  horizontal_blur.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/8/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the texture containing the original color
uniform sampler2D originalTexture;

void main(void) {
    float ds = dFdx(gl_TexCoord[0].s);
    gl_FragColor = (texture2D(originalTexture, gl_TexCoord[0].st + vec2(ds * -7.5, 0.0)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(ds * -5.5, 0.0)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(ds * -3.5, 0.0)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(ds * -1.5, 0.0)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(ds * 1.5, 0.0)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(ds * 3.5, 0.0)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(ds * 5.5, 0.0)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(ds * 7.5, 0.0))) / 8.0;
}
