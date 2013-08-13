#version 120

//
//  vertical_blur.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/8/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the texture containing the original color
uniform sampler2D originalTexture;

// the texture containing the horizontally blurred color
uniform sampler2D horizontallyBlurredTexture;

void main(void) {
    float dt = dFdy(gl_TexCoord[0].t);
    vec4 blurred = (texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * -15.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * -13.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * -11.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * -9.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * -7.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * -4.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * -3.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * -1.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * 1.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * 3.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * 5.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * 7.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * 9.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * 11.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * 13.5)) +
        texture2D(horizontallyBlurredTexture, gl_TexCoord[0].st + vec2(0.0, dt * 15.5))) / 16.0;
    gl_FragColor = blurred * blurred.a + texture2D(originalTexture, gl_TexCoord[0].st) * (1.0 + blurred.a * 0.5);
}
