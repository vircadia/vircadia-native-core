#version 120

//
//  glow_add.frag
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

void main(void) {
    vec4 color = texture2D(originalTexture, gl_TexCoord[0].st);
    gl_FragColor = color * (1.0 + color.a);
}
