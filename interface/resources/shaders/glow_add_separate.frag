#version 120

//
//  glow_add_separate.frag
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

// the texture containing the blurred color
uniform sampler2D blurredTexture;

void main(void) {
    vec4 blurred = texture2D(blurredTexture, gl_TexCoord[0].st);
    gl_FragColor = blurred * blurred.a + texture2D(originalTexture, gl_TexCoord[0].st) * (1.0 + blurred.a * 0.5);
}
