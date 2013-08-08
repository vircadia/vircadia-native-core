#version 120

//
//  horizontal_blur.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/8/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the texture containing the color to blur
uniform sampler2D colorTexture;

void main(void) {
    float ds = dFdx(gl_TexCoord[0].s);
    gl_FragColor = (texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * -15.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * -13.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * -11.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * -9.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * -7.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * -5.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * -3.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * -1.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * 1.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * 3.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * 5.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * 7.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * 9.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * 11.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * 13.5, 0.0)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(ds * 15.5, 0.0))) / 16.0;
}
