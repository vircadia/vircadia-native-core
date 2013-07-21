#version 120

//
//  face.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 7/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the color texture
uniform sampler2D colorTexture;

void main(void) {
    // for now, just modulate color
    gl_FragColor = gl_Color * texture2D(colorTexture, gl_TexCoord[0].st);
}
