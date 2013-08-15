#version 120

//
//  glow_add.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the texture containing the original color
uniform sampler2D originalTexture;

void main(void) {
    vec4 color = texture2D(originalTexture, gl_TexCoord[0].st);
    gl_FragColor = color * (1.0 + color.a);
}
