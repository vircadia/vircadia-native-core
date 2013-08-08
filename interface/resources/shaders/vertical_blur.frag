#version 120

//
//  vertical_blur.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/8/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the texture containing the color to blur
uniform sampler2D colorTexture;

void main(void) {
    float dt = dFdy(gl_TexCoord[0].t);
    gl_FragColor = (texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * -15.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * -13.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * -11.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * -9.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * -7.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * -4.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * -3.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * -1.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * 1.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * 3.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * 5.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * 7.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * 9.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * 11.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * 13.5)) +
        texture2D(colorTexture, gl_TexCoord[0].st + vec2(0.0, dt * 15.5))) / 16.0;
}
