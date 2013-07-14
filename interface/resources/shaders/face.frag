#version 120

//
//  iris.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 6/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the color texture
uniform sampler2D colorTexture;

void main(void) {
    // modulate texture by diffuse color and add specular contribution
    gl_FragColor = gl_Color * texture2D(colorTexture, gl_TexCoord[0].st);
}
