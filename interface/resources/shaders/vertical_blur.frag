#version 120

//
//  vertical_blur.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the texture containing the horizontally blurred color
uniform sampler2D originalTexture;

void main(void) {
    float dt = dFdy(gl_TexCoord[0].t);
    gl_FragColor = (texture2D(originalTexture, gl_TexCoord[0].st + vec2(0.0, dt * -7.5)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(0.0, dt * -5.5)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(0.0, dt * -3.5)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(0.0, dt * -1.5)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(0.0, dt * 1.5)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(0.0, dt * 3.5)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(0.0, dt * 5.5)) +
        texture2D(originalTexture, gl_TexCoord[0].st + vec2(0.0, dt * 7.5))) / 8.0;
}
