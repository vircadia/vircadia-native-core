#version 120

//
//  shadow_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 11/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

uniform sampler2DShadow shadowMap;

void main(void) {
    gl_FragColor = gl_Color * mix(vec4(0.8, 0.8, 0.8, 1.0), vec4(1.0, 1.0, 1.0, 1.0),
        shadow2D(shadowMap, gl_TexCoord[0].stp));
}
